#ifndef DISCOVERY_DISCOVERY_HPP_
#define DISCOVERY_DISCOVERY_HPP_

#include "ripple/discovery/disco_packet.hpp"
#include "ripple/discovery/peer_manager.hpp"
#include "ripple/logger/logger.hpp"
#include "ripple/transport/multicast/mcast.hpp"
#include "ripple/transport/packet/controller.hpp"
#include "ripple/transport/packet/endpoint.hpp"
#include "ripple/util/cert/identity.hpp"
#include <boost/signals2/connection.hpp>
#include <memory>

namespace ripple::discovery {

class DiscoveryNode {
private:
  static constexpr int DISCO_INTERVAL_MS = 1000;

  std::shared_ptr<logger::logger> logger;

  util::cert::id_ptr identity;
  std::shared_ptr<transport::multicast::MulticastTransport> transport;
  std::shared_ptr<transport::packet::PacketController> controller;
  std::shared_ptr<PeerManager> manager;
  boost::signals2::scoped_connection rx_connection;

  // timer to control how often to send disco packets
  std::unique_ptr<boost::asio::steady_timer> disco_timer;
  std::chrono::milliseconds disco_interval;

  long start_millis;
  int target_port;

  long get_epoch_millis();

  std::shared_ptr<ripple::discovery::DiscoPacket> create_pkt();
  void transmit_pkt();

  void disco_timer_act();

  // messages rx'd over transport
  void net_callback(const std::shared_ptr<transport::packet::Message> msg);

  void
  update_peer_from_disco(peer_ptr peer,
                         std::shared_ptr<ripple::discovery::DiscoPacket> pkt,
                         transport::packet::Endpoint endpoint);

public:
  DiscoveryNode(
      int target_port, util::cert::id_ptr identity,
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<transport::multicast::MulticastTransport> transport,
      std::shared_ptr<transport::packet::PacketController> controller,
      std::shared_ptr<PeerManager> manager);
  ~DiscoveryNode();
};

} // namespace ripple::discovery

#endif /* DISCOVERY_DISCOVERY_HPP_ */