#ifndef QUIC_CLIENT_HPP_
#define QUIC_CLIENT_HPP_

#include "ripple/transport/packet/endpoint.hpp"
#include "ripple/transport/quic/options.hpp"
#include <boost/signals2.hpp>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <msquic.hpp>

#include "ripple/logger/logger.hpp"
#include "ripple/transport/quic/api.hpp"
#include "ripple/util/cert/identity.hpp"

namespace ripple::transport::quic {

struct QuicPeer {
  packet::Endpoint endpoint;
  std::vector<uint8_t> expected_spki_hash;

  std::condition_variable connection_state_change;
  std::mutex connection_state_mutex;

  bool connected = false;
  bool shutdown = false;

  // unused?
  QUIC_STATUS shutdown_status = QUIC_STATUS_SUCCESS;
};

class QuicClient {

private:
  struct ClientCallbackContext {
    QuicClient *client;
    QuicPeer peer;
  };

  struct ActiveConnection {
    ClientCallbackContext context{};
    std::unique_ptr<MsQuicConnection> connection;
  };

  struct PendingDatagramSend {
    std::vector<uint8_t> payload;
    QUIC_BUFFER buffer{};
  };

  struct PendingStreamSend {
    std::vector<uint8_t> payload;
    QUIC_BUFFER buffer{};
  };

  std::shared_ptr<logger::logger> logger;

  util::cert::id_ptr identity;

  std::shared_ptr<MsQuicApi> api;
  QuicOptions opt;

  QuicAddr addr;

  // Keep PKCS#12 backing memory alive for as long as MsQuic may read it.
  std::vector<uint8_t> identity_pkcs12_blob;
  QUIC_CERTIFICATE_PKCS12 identity_pkcs12{};

  std::mutex active_connections_mutex;
  std::vector<std::unique_ptr<ActiveConnection>> active_connections;

  std::mutex pending_datagrams_mutex;
  std::unordered_map<void *, std::unique_ptr<PendingDatagramSend>>
      pending_datagram_payloads;

  std::unique_ptr<MsQuicRegistration> registration;
  std::unique_ptr<MsQuicConfiguration> configuration;
  bool initialized = false;

  bool protocol_init();
  QUIC_CREDENTIAL_CONFIG init_create_cred_config();
  void reap_closed_connections();
  void shutdown_active_connections();

  // Returns the connected, non-shutdown MsQuicConnection for an endpoint,
  // or nullptr. Caller must hold active_connections_mutex.
  MsQuicConnection *find_connection(const packet::Endpoint &endpoint);

  // #region MsQuic callbacks
  static QUIC_STATUS QUIC_API quic_conn_callback(MsQuicConnection *conn,
                                                 void *ctx_ptr,
                                                 QUIC_CONNECTION_EVENT *ev);

public:
  boost::signals2::signal<void(const packet::Endpoint &, bool)>
      connection_state_ev;
  boost::signals2::signal<void(const packet::Endpoint &,
                               const std::vector<uint8_t> &)>
      datagram_received_ev;

  QuicClient(QuicOptions &opt, util::cert::id_ptr identity,
             std::shared_ptr<MsQuicApi> api);
  ~QuicClient();

  bool add_endpoint(const packet::Endpoint &endpoint);

  // Send a QUIC datagram (unreliable, must fit in one packet) to an endpoint.
  bool send_datagram(const packet::Endpoint &endpoint,
                     const std::vector<uint8_t> &data);

  // Open a unidirectional stream to an endpoint and send data.
  bool send_stream(const packet::Endpoint &endpoint,
                   const std::vector<uint8_t> &data);
};
} // namespace ripple::transport::quic

#endif /* QUIC_CLIENT_HPP_*/