#ifndef QUIC_QUIC_HPP_
#define QUIC_QUIC_HPP_

#define QUIC_API_ENABLE_PREVIEW_FEATURES 1
#include <msquic.hpp>

#include "ripple/logger/logger.hpp"
#include "ripple/util/cert/identity.hpp"

namespace ripple::transport::quic {

struct QuicOptions {
  std::string protocol_name = "ripple-quic";
  std::string alpn = "ripple";
  uint64_t idle_timeout_ms = 5000;
  uint32_t keep_alive_ms = 1000;
  uint16_t peer_stream_bidirectional_count = 100;
  uint16_t peer_stream_unidirectional_count = 100;
};

class QuicTransport {

private:
  static constexpr QUIC_SERVER_RESUMPTION_LEVEL RESUMPTION_LEVEL =
      QUIC_SERVER_RESUME_AND_ZERORTT;

  static constexpr bool DATAGRAM_RX_ENABLED = true;

  std::shared_ptr<logger::logger> logger;

  util::cert::id_ptr identity;

  std::unique_ptr<MsQuicApi> api;
  QuicOptions opt;

  QuicAddr addr;

  // Keep PKCS#12 backing memory alive for as long as MsQuic may read it.
  std::vector<uint8_t> identity_pkcs12_blob;
  QUIC_CERTIFICATE_PKCS12 identity_pkcs12{};

  std::unique_ptr<MsQuicRegistration> registration;
  std::unique_ptr<MsQuicConfiguration> configuration;
  std::unique_ptr<MsQuicAutoAcceptListener> listener;

  bool protocol_init();
  QUIC_CREDENTIAL_CONFIG init_create_cred_config();

  // #region MsQuic callbacks
  static QUIC_STATUS QUIC_API quic_conn_callback(MsQuicConnection *conn,
                                                 void *ctx,
                                                 QUIC_CONNECTION_EVENT *ev);

public:
  QuicTransport(QuicOptions &opt, util::cert::id_ptr identity);
  ~QuicTransport();

  uint16_t get_port();
};
} // namespace ripple::transport::quic

#endif /* QUIC_QUIC_HPP_*/