#include "ripple/transport/quic/client.hpp"
#include "msquic.h"
#include "ripple/transport/packet/endpoint.hpp"
#include "ripple/util/cert/common.hpp"

#include <algorithm>
#include <memory>

namespace ripple::transport::quic {

QuicClient::QuicClient(QuicOptions &opt, util::cert::id_ptr identity,
                       std::shared_ptr<MsQuicApi> api) {
  this->opt = opt;
  this->identity = identity;
  this->api = api;

  logger =
      logger::LoggerProvider::get_logger("ripple::transport::quic::QuicClient");

  logger->info("Starting QUIC client");

  this->api = acquire_msquic_api(std::move(this->api));
  if (!this->api) {
    logger->critical("MsQuicApi not provided to client");
    return;
  }
  initialized = protocol_init();

  if (!initialized) {
    logger->critical("QUIC client initialization failed");
  }
};

QuicClient::~QuicClient() {
  shutdown_active_connections();
  {
    std::lock_guard<std::mutex> guard(pending_datagrams_mutex);
    pending_datagram_payloads.clear();
  }
  initialized = false;
  configuration.reset();
  registration.reset();
}

void QuicClient::reap_closed_connections() {
  std::lock_guard<std::mutex> guard(active_connections_mutex);
  active_connections.erase(
      std::remove_if(active_connections.begin(), active_connections.end(),
                     [](const std::unique_ptr<ActiveConnection> &connection) {
                       return connection->context.peer.shutdown;
                     }),
      active_connections.end());
}

void QuicClient::shutdown_active_connections() {
  std::vector<std::unique_ptr<ActiveConnection>> draining_connections;
  {
    std::lock_guard<std::mutex> guard(active_connections_mutex);
    draining_connections.swap(active_connections);
  }

  for (auto &active_connection : draining_connections) {
    if (!active_connection->connection) {
      continue;
    }

    active_connection->connection->Callback = MsQuicConnection::NoOpCallback;
    active_connection->connection->Context = nullptr;
    active_connection->connection->Shutdown(
        0, QUIC_CONNECTION_SHUTDOWN_FLAG_SILENT);
    active_connection->connection->Close();
  }
}

bool QuicClient::protocol_init() {

  // Register (defaults to low latency but explicitly set here for ease of use)
  registration = std::make_unique<MsQuicRegistration>(
      identity->get_cn().c_str(), QUIC_EXECUTION_PROFILE_LOW_LATENCY);
  if (!registration->IsValid()) {
    logger->critical("created MsQuicRegistration was not valid.");
    return false;
  }

  // settings
  MsQuicSettings settings;
  settings.SetIdleTimeoutMs(opt.idle_timeout_ms)
      .SetKeepAlive(opt.keep_alive_ms)
      .SetPeerBidiStreamCount(opt.peer_stream_bidirectional_count)
      .SetServerResumptionLevel(QUIC_SERVER_RESUME_AND_ZERORTT);

  // Load credentials from identity
  QUIC_CREDENTIAL_CONFIG creds = init_create_cred_config();

  MsQuicAlpn alpn(opt.alpn.c_str());

  // create a quic config with all our opts
  configuration = std::make_unique<MsQuicConfiguration>(
      *registration, alpn, settings, MsQuicCredentialConfig(creds));
  if (!configuration->IsValid()) {
    logger->critical("config load failed, check cert");
    return false;
  }

  return true;
};

QUIC_CREDENTIAL_CONFIG QuicClient::init_create_cred_config() {
  // convert identity keypair into pkcs12 blob (in mem)
  identity_pkcs12_blob = identity->pkcs12_blob();

  identity_pkcs12.Asn1Blob = identity_pkcs12_blob.data();
  identity_pkcs12.Asn1BlobLength =
      static_cast<uint32_t>(identity_pkcs12_blob.size());
  identity_pkcs12.PrivateKeyPassword = nullptr;

  // create cred config struct
  QUIC_CREDENTIAL_CONFIG cred{};
  cred.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_PKCS12;
  cred.CertificatePkcs12 = &identity_pkcs12;
  cred.Flags = QUIC_CREDENTIAL_FLAG_CLIENT |
               QUIC_CREDENTIAL_FLAG_INDICATE_CERTIFICATE_RECEIVED |
               QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION; // client

  // return ref
  return cred;
};

bool QuicClient::add_endpoint(packet::Endpoint &endpoint) {
  reap_closed_connections();

  if (!initialized || !registration || !configuration) {
    logger->critical("QUIC client is not initialized");
    return false;
  }

  auto active_connection = std::make_unique<ActiveConnection>();
  active_connection->context.client = this;
  active_connection->context.peer.endpoint = endpoint;

  active_connection->connection = std::make_unique<MsQuicConnection>(
      *registration, CleanUpManual, quic_conn_callback,
      &active_connection->context);

  if (!active_connection->connection->IsValid()) {
    logger->critical("ConnectionOpen failed");
    return false;
  }

  const auto endpoint_host = endpoint.address.to_string();
  if (QUIC_FAILED(active_connection->connection->Start(
          *configuration, endpoint_host.c_str(), endpoint.port))) {

    logger->critical("ConnectionStart failed");
    return false;
  }

  {
    std::lock_guard<std::mutex> guard(active_connections_mutex);
    active_connections.push_back(std::move(active_connection));
  }

  return true;
};

QUIC_STATUS QUIC_API QuicClient::quic_conn_callback(MsQuicConnection *conn,
                                                    void *ctx_ptr,
                                                    QUIC_CONNECTION_EVENT *ev) {
  // cast ctx back to struct
  auto *ctx = static_cast<ClientCallbackContext *>(ctx_ptr);

  switch (ev->Type) {
  case QUIC_CONNECTION_EVENT_CONNECTED: {
    ctx->client->logger->info("[conn {}]: connected",
                              ctx->peer.endpoint.to_string());

    // in mutex, mark as connected
    {
      std::lock_guard<std::mutex> guard(ctx->peer.connection_state_mutex);
      ctx->peer.connected = true;
    }

    ctx->peer.connection_state_change.notify_all();
    ctx->client->connection_state_ev(ctx->peer.endpoint, true);
    break;
  }
  case QUIC_CONNECTION_EVENT_PEER_CERTIFICATE_RECEIVED: {
    // spki pinning (from eg disco pkt)

    // if expected empty, accept anything
    if (ctx->peer.expected_spki_hash.empty()) {
      return QUIC_STATUS_SUCCESS;
    }

    // get peer cert
    util::cert::cert_ptr peer_cert(
        static_cast<X509 *>(ev->PEER_CERTIFICATE_RECEIVED.Certificate));

    if (!peer_cert) {
      ctx->client->logger->critical("no peer cert");
      return QUIC_STATUS_BAD_CERTIFICATE;
    }
    auto got = util::cert::hash_cert_spki(peer_cert);
    if (got != ctx->peer.expected_spki_hash) {
      ctx->client->logger->critical("SPKI hash mismatch — possible MITM");
      return QUIC_STATUS_BAD_CERTIFICATE;
    }
    ctx->client->logger->info("[conn {}] peer SPKI verified",
                              ctx->peer.endpoint.to_string());
    break;
  }

  case QUIC_CONNECTION_EVENT_DATAGRAM_RECEIVED: {
    std::vector<uint8_t> payload;
    if (ev->DATAGRAM_RECEIVED.Buffer && ev->DATAGRAM_RECEIVED.Buffer->Buffer &&
        ev->DATAGRAM_RECEIVED.Buffer->Length > 0) {
      const auto *begin = ev->DATAGRAM_RECEIVED.Buffer->Buffer;
      payload.assign(begin, begin + ev->DATAGRAM_RECEIVED.Buffer->Length);
    }

    ctx->client->logger->info("[conn {}] rx datagram of {} bytes",
                              ctx->peer.endpoint.to_string(),
                              ev->DATAGRAM_RECEIVED.Buffer->Length);

    ctx->client->datagram_received_ev(ctx->peer.endpoint, payload);

    break;
  }
  case QUIC_CONNECTION_EVENT_DATAGRAM_SEND_STATE_CHANGED: {
    void *send_ctx = ev->DATAGRAM_SEND_STATE_CHANGED.ClientContext;
    if (send_ctx && QUIC_DATAGRAM_SEND_STATE_IS_FINAL(
                        ev->DATAGRAM_SEND_STATE_CHANGED.State)) {
      std::lock_guard<std::mutex> guard(ctx->client->pending_datagrams_mutex);
      ctx->client->pending_datagram_payloads.erase(send_ctx);
    }
    break;
  }
  case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT: {

    ctx->client->logger->info("[conn {}] transport shutdown {}",
                              ctx->peer.endpoint.to_string(),
                              ev->SHUTDOWN_INITIATED_BY_TRANSPORT.Status);

    ctx->peer.shutdown_status = ev->SHUTDOWN_INITIATED_BY_TRANSPORT.Status;
    break;
  }
  case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER: {
    ctx->client->logger->info("[conn {}] peer-initiated shutdown {}",
                              ctx->peer.endpoint.to_string());
    break;
  }
  case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE: {
    std::lock_guard<std::mutex> lk(ctx->peer.connection_state_mutex);
    ctx->peer.shutdown = true;
    ctx->peer.connection_state_change.notify_all();
    ctx->client->connection_state_ev(ctx->peer.endpoint, false);
    break;
  }

  default:
    break;
  }
  return QUIC_STATUS_SUCCESS;
};

MsQuicConnection *
QuicClient::find_connection(const packet::Endpoint &endpoint) {
  // Caller must hold active_connections_mutex.
  for (auto &ac : active_connections) {
    if (ac->context.peer.endpoint == endpoint && ac->context.peer.connected &&
        !ac->context.peer.shutdown) {
      return ac->connection.get();
    }
  }
  return nullptr;
}

bool QuicClient::send_datagram(const packet::Endpoint &endpoint,
                               const std::vector<uint8_t> &data) {
  MsQuicConnection *conn = nullptr;
  {
    std::lock_guard<std::mutex> guard(active_connections_mutex);
    conn = find_connection(endpoint);
  }

  if (!conn) {
    logger->warn("send_datagram: no connected peer {}", endpoint.to_string());
    return false;
  }

  // MsQuic can complete datagram send asynchronously, so keep payload alive
  // until DATAGRAM_SEND_STATE_CHANGED reaches a final state.
  auto pending = std::make_unique<PendingDatagramSend>();
  pending->payload = data;
  pending->buffer.Buffer = pending->payload.data();
  pending->buffer.Length = static_cast<uint32_t>(pending->payload.size());

  void *send_ctx = pending.get();

  {
    std::lock_guard<std::mutex> guard(pending_datagrams_mutex);
    pending_datagram_payloads[send_ctx] = std::move(pending);
  }

  QUIC_BUFFER *buf = nullptr;
  {
    std::lock_guard<std::mutex> guard(pending_datagrams_mutex);
    auto it = pending_datagram_payloads.find(send_ctx);
    if (it == pending_datagram_payloads.end()) {
      logger->error("send_datagram: payload tracking failed");
      return false;
    }
    buf = &it->second->buffer;
  }

  QUIC_STATUS status =
      MsQuic->DatagramSend(*conn, buf, 1, QUIC_SEND_FLAG_NONE, send_ctx);
  if (QUIC_FAILED(status)) {
    std::lock_guard<std::mutex> guard(pending_datagrams_mutex);
    pending_datagram_payloads.erase(send_ctx);
    logger->error("send_datagram: DatagramSend failed 0x{:x}", status);
    return false;
  }
  return true;
}

bool QuicClient::send_stream(const packet::Endpoint &endpoint,
                             const std::vector<uint8_t> &data) {
  MsQuicConnection *conn = nullptr;
  {
    std::lock_guard<std::mutex> guard(active_connections_mutex);
    conn = find_connection(endpoint);
  }

  if (!conn) {
    logger->warn("send_stream: no connected peer {}", endpoint.to_string());
    return false;
  }

  auto *pending = new PendingStreamSend();
  pending->payload = data;
  pending->buffer.Buffer = pending->payload.data();
  pending->buffer.Length = static_cast<uint32_t>(pending->payload.size());

  // CleanUpAutoDelete: MsQuic closes the stream handle after SHUTDOWN_COMPLETE.
  auto *stream = new MsQuicStream(
      *conn, QUIC_STREAM_OPEN_FLAG_UNIDIRECTIONAL, CleanUpAutoDelete,
      [](MsQuicStream *s, void *ctx, QUIC_STREAM_EVENT *ev) -> QUIC_STATUS {
        auto *pending_send = static_cast<PendingStreamSend *>(ctx);
        if (!pending_send) {
          return QUIC_STATUS_SUCCESS;
        }

        if (ev->Type == QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE) {
          s->Context = nullptr;
          delete pending_send;
        }

        return QUIC_STATUS_SUCCESS;
      },
      pending);

  if (!stream->IsValid()) {
    logger->error("send_stream: StreamOpen failed");
    delete pending;
    delete stream;
    return false;
  }

  if (QUIC_FAILED(stream->Start(QUIC_STREAM_START_FLAG_IMMEDIATE))) {
    logger->error("send_stream: StreamStart failed");
    delete pending;
    delete stream;
    return false;
  }

  QUIC_STATUS status =
      stream->Send(&pending->buffer, 1, QUIC_SEND_FLAG_FIN, pending);
  if (QUIC_FAILED(status)) {
    logger->error("send_stream: StreamSend failed 0x{:x}", status);
    delete pending;
    stream->Context = nullptr;
    // Stream is already started; shut it down rather than double-delete.
    stream->ConnectionShutdown(1);
    return false;
  }
  return true;
}

} // namespace ripple::transport::quic