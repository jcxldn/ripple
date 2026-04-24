#ifndef DISCOVERY_NODE_HPP_
#define DISCOVERY_NODE_HPP_

#include "ripple/discovery/discovery.hpp"
#include "ripple/logger/logger.hpp"
#include "ripple/transport/multicast/mcast.hpp"
#include "ripple/transport/quic/quic.hpp"
#include "ripple/util/cert/identity.hpp"

namespace ripple::discovery {

class Node {
private:
  std::shared_ptr<logger::logger> logger;

  util::cert::id_ptr id;

  std::shared_ptr<transport::multicast::MulticastTransport> mcast;
  std::shared_ptr<ripple::transport::quic::QuicTransport> quic;

  // child nodes share the same io context
  // transports have their own context
  std::shared_ptr<std::thread> context_thread;
  std::shared_ptr<boost::asio::io_context> io_context;
  void thread_loop();

  std::shared_ptr<DiscoveryNode> discovery;

public:
  Node();
  ~Node();
};

} // namespace ripple::discovery

#endif /* DISCOVERY_NODE_HPP_ */