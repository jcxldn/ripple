#ifndef CONNECTION_DISCOVERY_HPP_
#define CONNECTION_DISCOVERY_HPP_

#include "ripple/logger/logger.hpp"
#include "ripple/peer/peer.hpp"

#include "ripple/discovery/peer_manager.hpp"
#include "ripple/transport/multicast/mcast.hpp"
#include "ripple/transport/packet/controller.hpp"

#include "ripple/discovery/discovery.hpp"
namespace ripple::peer::connection {

class DiscoveryConnection {
private:
  std::shared_ptr<logger::logger> logger;

  Peer *peer;

  std::shared_ptr<discovery::PeerManager> peer_manager;
  void peer_added_handler(const discovery::peer_ptr discovered);

  boost::signals2::scoped_connection peer_added_connection;

  // multicast peer discovery (not tied to router)
  std::shared_ptr<transport::multicast::MulticastTransport> mcast;
  std::shared_ptr<ripple::transport::packet::PacketController> mcast_controller;

  std::shared_ptr<discovery::DiscoveryNode> discovery;

  // child nodes share the same io context
  // transports have their own context
  std::shared_ptr<std::thread> context_thread;
  std::shared_ptr<boost::asio::io_context> io_context;
  void thread_loop();

public:
  DiscoveryConnection(Peer *peer, uint16_t port);
  ~DiscoveryConnection();

  size_t known_peer_count() const;
};

} // namespace ripple::peer::connection

#endif /* CONNECTION_DISCOVERY_HPP_ */
