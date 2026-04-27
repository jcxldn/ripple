#ifndef QUIC_QUIC_HPP_
#define QUIC_QUIC_HPP_

#include "ripple/transport/packet/endpoint.hpp"
#include "ripple/transport/quic/options.hpp"
#include <boost/signals2.hpp>
#include <msquic.hpp>

#include "ripple/logger/logger.hpp"
#include "ripple/transport/quic/api.hpp"
#include "ripple/util/cert/identity.hpp"

#include <condition_variable>
#include <mutex>
#include <unordered_set>

namespace ripple::transport::quic {

class QuicTransport {

private:
  static constexpr QUIC_SERVER_RESUMPTION_LEVEL RESUMPTION_LEVEL =
      QUIC_SERVER_RESUME_AND_ZERORTT;

  static constexpr bool DATAGRAM_RX_ENABLED = true;

  std::shared_ptr<logger::logger> logger;

  util::cert::id_ptr identity;

  std::shared_ptr<MsQuicApi> api;
  QuicOptions opt;

  QuicAddr addr;

  // Keep PKCS#12 backing memory alive for as long as MsQuic may read it.
  std::vector<uint8_t> identity_pkcs12_blob;
  QUIC_CERTIFICATE_PKCS12 identity_pkcs12{};

  std::unique_ptr<MsQuicRegistration> registration;
  std::unique_ptr<MsQuicConfiguration> configuration;
  std::unique_ptr<MsQuicAutoAcceptListener> listener;

  std::mutex active_connections_mutex;
  std::condition_variable active_connections_drained;
  std::unordered_set<MsQuicConnection *> active_connections;
  bool shutting_down = false;

  struct StreamCallbackContext {
    QuicTransport *transport = nullptr;
    transport::packet::Endpoint remote_endpoint{};
  };

  bool protocol_init();
  QUIC_CREDENTIAL_CONFIG init_create_cred_config();

  // #region MsQuic callbacks
  static QUIC_STATUS QUIC_API quic_conn_callback(MsQuicConnection *conn,
                                                 void *ctx,
                                                 QUIC_CONNECTION_EVENT *ev);

  static QUIC_STATUS QUIC_API quic_stream_callback(MsQuicStream *stream,
                                                   void *ctx,
                                                   QUIC_STREAM_EVENT *ev);

public:
  boost::signals2::signal<void(const transport::packet::Endpoint &,
                               const std::vector<uint8_t> &)>
      datagram_received_ev;
  boost::signals2::signal<void(const transport::packet::Endpoint &,
                               const std::vector<uint8_t> &)>
      stream_received_ev;

  QuicTransport(QuicOptions &opt, util::cert::id_ptr identity);
  ~QuicTransport();

  uint16_t get_port();

  inline std::shared_ptr<MsQuicApi> get_api() { return api; };
};
} // namespace ripple::transport::quic

#endif /* QUIC_QUIC_HPP_*/