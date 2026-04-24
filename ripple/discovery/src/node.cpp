#include "ripple/discovery/node.hpp"
#include "ripple/discovery/discovery.hpp"
#include "ripple/transport/multicast/mcast.hpp"
#include "ripple/util/cert/identity.hpp"
#include <memory>

namespace ripple::discovery {

Node::Node() {

  logger = logger::LoggerProvider::get_logger("ripple::discovery::Node");

  // Generate an identity (cert / key self signed pair)
  id = std::make_shared<util::cert::Identity>();

  logger->info("Cert hash: {}", id->spki_b64());

  // create an io_context (also used by child nodes)
  io_context = std::make_shared<boost::asio::io_context>();

  // create quic
  transport::quic::QuicOptions quic_options;
  quic = std::make_shared<transport::quic::QuicTransport>(quic_options, id);

  // Create a multicast transport to send discovery ("disco") packets over

  // use default options
  ripple::transport::multicast::MulticastOptions mcast_options;

  mcast =
      std::make_shared<transport::multicast::MulticastTransport>(mcast_options);

  // Create a discovery node
  discovery =
      std::make_shared<DiscoveryNode>(quic->get_port(), id, io_context, mcast);

  context_thread = std::make_shared<std::thread>(&Node::thread_loop, this);
};

Node::~Node() {
  io_context->stop();

  // wait for io_context to stop
  if (context_thread && context_thread->joinable())
    context_thread->join();
};

void Node::thread_loop() { io_context->run(); };

} // namespace ripple::discovery