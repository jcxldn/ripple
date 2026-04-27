#include "ripple/transport/quic/quic.hpp"
#include "msquic.h"
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
  listener.reset();
  configuration.reset();
  registration.reset();

  if (MsQuic == api.get()) {
    MsQuic = nullptr;
  }

  api.reset();
}

bool QuicTransport::protocol_init() {
  api = std::make_shared<MsQuicApi>();
  if (QUIC_FAILED(api->GetInitStatus())) {
    logger->critical("MsQuicApi init failed!");
    return false;
  }

  // set MsQuic's global pointer
  MsQuic = api.get();

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
  QuicTransport *qt = (QuicTransport *)ctx;
  switch (ev->Type) {
  case QUIC_CONNECTION_EVENT_CONNECTED:
    qt->logger->info("[conn {}]: connected", (void *)conn);
    break;
  default:
    break;
  }
  return QUIC_STATUS_SUCCESS;
};

} // namespace ripple::transport::quic