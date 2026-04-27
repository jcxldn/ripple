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

} // namespace ripple::transport::quic