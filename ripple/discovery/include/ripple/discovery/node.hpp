#ifndef DISCOVERY_NODE_HPP_
#define DISCOVERY_NODE_HPP_

#include "ripple/discovery/discovery.hpp"
#include "ripple/discovery/peer_manager.hpp"
#include "ripple/logger/logger.hpp"
#include "ripple/peer/router.hpp"
#include "ripple/transport/multicast/mcast.hpp"
#include "ripple/transport/packet/controller.hpp"
#include "ripple/transport/quic/client.hpp"
#include "ripple/transport/quic/quic.hpp"
#include "ripple/util/cert/identity.hpp"
#include <boost/signals2/connection.hpp>

namespace ripple::discovery {

class Node {
private:
  std::shared_ptr<logger::logger> logger;

  util::cert::id_ptr id;

  std::shared_ptr<transport::multicast::MulticastTransport> mcast;
  std::shared_ptr<ripple::transport::packet::PacketController> mcast_controller;
  std::shared_ptr<ripple::transport::quic::QuicTransport> quic;
  std::shared_ptr<ripple::transport::quic::QuicClient> quic_client;

  // child nodes share the same io context
  // transports have their own context
  std::shared_ptr<std::thread> context_thread;
  std::shared_ptr<boost::asio::io_context> io_context;
  void thread_loop();

  std::shared_ptr<DiscoveryNode> discovery;

  std::shared_ptr<PeerManager> peer_manager;
  std::shared_ptr<ripple::peer::Router> peer_router;
  boost::signals2::scoped_connection peer_added_connection;
  boost::signals2::scoped_connection quic_state_connection;
  boost::signals2::scoped_connection quic_receive_connection;
  boost::signals2::scoped_connection quic_transport_receive_connection;
  boost::signals2::scoped_connection quic_transport_stream_receive_connection;
  boost::signals2::scoped_connection mcast_receive_connection;

  void peer_added_handler(const peer_ptr peer);
  void
  quic_connection_state_handler(const transport::packet::Endpoint &endpoint,
                                bool connected);

public:
  Node();
  ~Node();

  size_t known_peer_count() const;

  inline std::shared_ptr<PeerManager> get_peer_manager() {
    return peer_manager;
  };

  inline std::shared_ptr<ripple::peer::Router> get_peer_router() {
    return peer_router;
  }

  inline std::shared_ptr<ripple::transport::quic::QuicClient>
  get_quic_client() {
    return quic_client;
  }
};

} // namespace ripple::discovery

#endif /* DISCOVERY_NODE_HPP_ */