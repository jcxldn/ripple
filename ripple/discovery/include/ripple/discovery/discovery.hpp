#ifndef DISCOVERY_DISCOVERY_HPP_
#define DISCOVERY_DISCOVERY_HPP_

#include "ripple/discovery/disco_packet.hpp"
#include "ripple/logger/logger.hpp"
#include "ripple/transport/multicast/mcast.hpp"
#include "ripple/util/cert/identity.hpp"

namespace ripple::discovery {

class DiscoveryNode {
private:
  static constexpr int DISCO_INTERVAL_MS = 1000;

  std::shared_ptr<logger::logger> logger;

  util::cert::id_ptr identity;
  std::shared_ptr<transport::multicast::MulticastTransport> transport;

  // timer to control how often to send disco packets
  boost::asio::steady_timer *disco_timer;
  std::chrono::milliseconds disco_interval;

  long start_millis;
  int target_port;

  long get_epoch_millis();

  std::shared_ptr<ripple::discovery::DiscoPacket> create_pkt();
  void transmit_pkt();

  void disco_timer_act();

public:
  DiscoveryNode(
      int target_port, util::cert::id_ptr identity,
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<transport::multicast::MulticastTransport> transport);
};

} // namespace ripple::discovery

#endif /* DISCOVERY_DISCOVERY_HPP_ */