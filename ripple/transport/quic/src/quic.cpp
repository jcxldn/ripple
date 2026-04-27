#include "ripple/transport/quic/quic.hpp"
#include "msquic.h"
#include <chrono>
#include <memory>

namespace ripple::transport::quic {

QuicTransport::QuicTransport(QuicOptions &opt, util::cert::id_ptr identity) {
  this->opt = opt;
  this->identity = identity;

  logger = logger::LoggerProvider::get_logger("ripple::transport::quic");

  logger->info("Starting QUIC transport");

  bool success = protocol_init();

  if (success) {
    logger->info("QUIC transport ready.");
  }
};

QuicTransport::~QuicTransport() {
  {
    std::lock_guard<std::mutex> guard(active_connections_mutex);
    shutting_down = true;
  }

  listener.reset();

  if (registration) {
    registration->Shutdown(QUIC_CONNECTION_SHUTDOWN_FLAG_SILENT, 1);

    std::unique_lock<std::mutex> guard(active_connections_mutex);
    active_connections_drained.wait_for(guard, std::chrono::seconds(1), [this] {
      return active_connections.empty();
    });
  }

  configuration.reset();
  registration.reset();

  api.reset();
}

bool QuicTransport::protocol_init() {
  api = acquire_msquic_api();
  if (!api) {
    logger->critical("MsQuicApi init failed!");
    return false;
  }

  // Register (defaults to low latency but explicitly set here for ease of use)
  registration = std::make_unique<MsQuicRegistration>(
      opt.protocol_name.c_str(), QUIC_EXECUTION_PROFILE_LOW_LATENCY);
  if (!registration->IsValid()) {
    logger->critical("created MsQuicRegistration was not valid.");
    return false;
  }

  // settings
  MsQuicSettings settings;
  settings.SetIdleTimeoutMs(opt.idle_timeout_ms)
      .SetKeepAlive(opt.keep_alive_ms)
      .SetPeerBidiStreamCount(opt.peer_stream_bidirectional_count)
      .SetPeerUnidiStreamCount(opt.peer_stream_unidirectional_count)
      .SetDatagramReceiveEnabled(DATAGRAM_RX_ENABLED)
      .SetServerResumptionLevel(RESUMPTION_LEVEL);

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

  // create listener
  listener = std::make_unique<MsQuicAutoAcceptListener>(
      *registration, *configuration, quic_conn_callback, this);

  // random port
  addr = QuicAddr(QUIC_ADDRESS_FAMILY_UNSPEC);

  if (QUIC_FAILED(listener->Start(alpn, &addr.SockAddr))) {
    logger->critical("Listener could not start.");
    return false;
  }

  logger->info("listening on udp *:{}, with alpn '{}'", get_port(), opt.alpn);

  return true;
};

QUIC_CREDENTIAL_CONFIG QuicTransport::init_create_cred_config() {
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
  cred.Flags = QUIC_CREDENTIAL_FLAG_NONE; // server

  // return ref
  return cred;
};

uint16_t QuicTransport::get_port() {
  QuicAddr addr;
  listener->GetLocalAddr(addr);
  uint16_t port = addr.GetPort();
  return port;
};

QUIC_STATUS QUIC_API QuicTransport::quic_conn_callback(
    MsQuicConnection *conn, void *ctx, QUIC_CONNECTION_EVENT *ev) {
  auto *qt = static_cast<QuicTransport *>(ctx);

  // Track from the very first event so connections that abort during the
  // handshake (before CONNECTED) are still visible to the drain wait in the
  // destructor. The unordered_set insertion is idempotent on repeat events.
  if (ev->Type != QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE) {
    std::lock_guard<std::mutex> guard(qt->active_connections_mutex);
    qt->active_connections.insert(conn);
  }

  switch (ev->Type) {
  case QUIC_CONNECTION_EVENT_CONNECTED:
    if (!qt->shutting_down) {
      qt->logger->info("[conn {}]: connected", static_cast<void *>(conn));
    }
    break;
  case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED: {
    // Create a stream object which is auto cleaned up
    // on shutdown with our callback
    new MsQuicStream(ev->PEER_STREAM_STARTED.Stream, CleanUpAutoDelete,
                     quic_stream_callback, qt);
    break;
  }
  case QUIC_CONNECTION_EVENT_DATAGRAM_RECEIVED: {
    const auto *b = ev->DATAGRAM_RECEIVED.Buffer;
    qt->logger->info("[conn {}] rx datagram {} bytes", (void *)conn, b->Length);
    break;
  }
  case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
    qt->logger->warn("[conn {}]: shutdown by transport, status=0x{:x} error={}",
                     (void *)conn, ev->SHUTDOWN_INITIATED_BY_TRANSPORT.Status,
                     ev->SHUTDOWN_INITIATED_BY_TRANSPORT.ErrorCode);
    break;
  case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
    qt->logger->warn("[conn {}]: shutdown by peer, error={}", (void *)conn,
                     ev->SHUTDOWN_INITIATED_BY_PEER.ErrorCode);
    break;
  case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE: {
    {
      std::lock_guard<std::mutex> guard(qt->active_connections_mutex);
      qt->active_connections.erase(conn);
    }
    qt->active_connections_drained.notify_all();
  } break;
  default:
    break;
  }
  return QUIC_STATUS_SUCCESS;
};

QUIC_STATUS QUIC_API QuicTransport::quic_stream_callback(
    MsQuicStream *stream, void *ctx, QUIC_STREAM_EVENT *ev) {
  auto *qt = static_cast<QuicTransport *>(ctx);
  switch (ev->Type) {
  case QUIC_STREAM_EVENT_RECEIVE: {
    uint64_t total = ev->RECEIVE.TotalBufferLength;
    qt->logger->info("[stream {}] rx {} bytes", static_cast<void *>(stream),
                     total);
    // Return SUCCESS to signal all bytes consumed; MsQuic will not deliver
    // them again. Use QUIC_STATUS_PENDING + StreamReceiveComplete() for async.
    // -> return QUIC_STATUS_SUCCESS when data consumed!
    break;
  }
  case QUIC_STREAM_EVENT_SEND_COMPLETE:
    // Send buffer returned to caller; Canceled=TRUE means it was not sent.
    if (ev->SEND_COMPLETE.Canceled) {
      qt->logger->warn("[stream {}] send canceled",
                       static_cast<void *>(stream));
    }
    break;
  case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
    // Peer sent FIN — graceful half-close of their send side.
    qt->logger->debug("[stream {}] peer send shutdown",
                      static_cast<void *>(stream));
    break;
  case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
    qt->logger->warn("[stream {}] peer send aborted, error={}",
                     static_cast<void *>(stream),
                     ev->PEER_SEND_ABORTED.ErrorCode);
    break;
  case QUIC_STREAM_EVENT_PEER_RECEIVE_ABORTED:
    qt->logger->warn("[stream {}] peer receive aborted, error={}",
                     static_cast<void *>(stream),
                     ev->PEER_RECEIVE_ABORTED.ErrorCode);
    break;
  case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
    // Stream is fully torn down. CleanUpAutoDelete frees the MsQuicStream.
    qt->logger->debug("[stream {}] shutdown complete",
                      static_cast<void *>(stream));
    break;
  default:
    break;
  }
  return QUIC_STATUS_SUCCESS;
}
} // namespace ripple::transport::quic