#include "ripple/peer/connection/discovery.hpp"

namespace ripple::peer::connection {

DiscoveryConnection::DiscoveryConnection(Peer *peer, uint16_t port) {
  this->peer = peer;

  logger =
      logger::LoggerProvider::get_logger("ripple::peer::connection::Discovery");

  // create an io_context (also used by child nodes)
  io_context = std::make_shared<boost::asio::io_context>();

  // create a peer manager instance
  peer_manager = std::make_shared<discovery::PeerManager>();

  // create a (non linked to router) multicast connection
  ripple::transport::multicast::MulticastOptions mcast_options;

  mcast =
      std::make_shared<transport::multicast::MulticastTransport>(mcast_options);

  // and a controller to resassemble mcast packets into messages
  mcast_controller =
      std::make_shared<ripple::transport::packet::PacketController>(
          mcast->get_io_context());

  // conect mcast to the controller
  mcast_controller->connect(mcast->rx_signal);

  // create a discovery node
  discovery = std::make_shared<discovery::DiscoveryNode>(
      port, peer->get_identity(), io_context, mcast, mcast_controller,
      peer_manager);

  // peer manager: handle new peers found
  peer_added_connection = peer_manager->peer_added_ev.connect(boost::bind(
      &DiscoveryConnection::peer_added_handler, this, std::placeholders::_1));

  // context thread to run the io_context for this
  context_thread =
      std::make_shared<std::thread>(&DiscoveryConnection::thread_loop, this);
};

DiscoveryConnection::~DiscoveryConnection() {
  peer_added_connection.disconnect();

  io_context->stop();

  // wait for io_context to stop
  if (context_thread && context_thread->joinable())
    context_thread->join();
};

void DiscoveryConnection::thread_loop() { io_context->run(); };

void DiscoveryConnection::peer_added_handler(
    const discovery::peer_ptr discovered) {
  logger->info("Handling new peer {}", discovered->endpoints.at(0).to_string());

  peer->get_router()->upsert_peer(discovered->hash, discovered->name,
                                  discovered->endpoints.at(0));
};

size_t DiscoveryConnection::known_peer_count() const {
  return peer_manager->count();
}
} // namespace ripple::peer::connection