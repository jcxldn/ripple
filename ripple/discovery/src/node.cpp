#include "ripple/discovery/node.hpp"
#include "ripple/discovery/discovery.hpp"
#include "ripple/discovery/peer_manager.hpp"
#include "ripple/peer/router.hpp"
#include "ripple/transport/multicast/mcast.hpp"
#include "ripple/transport/quic/client.hpp"
#include "ripple/util/cert/identity.hpp"
#include <memory>

namespace ripple::discovery {

Node::Node() {

  logger = logger::LoggerProvider::get_logger("ripple::discovery::Node");

  // create a peer mgr instance
  peer_manager = std::make_shared<PeerManager>();
  peer_router = std::make_shared<ripple::peer::Router>();

  // Generate an identity (cert / key self signed pair)
  id = std::make_shared<util::cert::Identity>();

  logger->info("Cert hash: {}", id->spki_b64());

  // create an io_context (also used by child nodes)
  io_context = std::make_shared<boost::asio::io_context>();

  // create quic
  transport::quic::QuicOptions quic_options;
  quic = std::make_shared<transport::quic::QuicTransport>(quic_options, id);

  // create quic client
  quic_client = std::make_shared<transport::quic::QuicClient>(quic_options, id,
                                                              quic->get_api());

  peer_router->register_sender(
      ripple::peer::TransportKind::quic,
      [client = quic_client](const ripple::peer::PeerRecord &peer,
                             const std::vector<uint8_t> &payload) {
        if (peer.endpoints.empty()) {
          return false;
        }
        return client->send_datagram(peer.endpoints.front(), payload);
      });

  // Create a multicast transport to send discovery ("disco") packets over

  // use default options
  ripple::transport::multicast::MulticastOptions mcast_options;

  mcast =
      std::make_shared<transport::multicast::MulticastTransport>(mcast_options);

  peer_router->register_sender(
      ripple::peer::TransportKind::multicast,
      [transport = mcast](const ripple::peer::PeerRecord &,
                          const std::vector<uint8_t> &payload) {
        auto bytes = std::make_shared<std::vector<uint8_t>>(payload);
        transport->transmit(bytes);
        return true;
      });

  // and a controller to resassemble mcast packets into messages
  mcast_controller =
      std::make_shared<ripple::transport::packet::PacketController>(
          mcast->get_io_context());

  // conect mcast to the controller
  mcast_controller->connect(mcast->rx_signal);

  // Create a discovery node
  discovery = std::make_shared<DiscoveryNode>(
      quic->get_port(), id, io_context, mcast, mcast_controller, peer_manager);

  context_thread = std::make_shared<std::thread>(&Node::thread_loop, this);

  // connect to sig for new node
  peer_added_connection = peer_manager->peer_added_ev.connect(
      boost::bind(&Node::peer_added_handler, this, std::placeholders::_1));

  quic_state_connection = quic_client->connection_state_ev.connect(
      boost::bind(&Node::quic_connection_state_handler, this,
                  std::placeholders::_1, std::placeholders::_2));

  quic_receive_connection = quic_client->datagram_received_ev.connect(
      [this](const transport::packet::Endpoint &endpoint,
             const std::vector<uint8_t> &payload) {
        peer_router->ingest_receive(ripple::peer::TransportKind::quic,
                                    ripple::peer::ReceiveKind::datagram,
                                    endpoint, payload);
      });

  quic_transport_receive_connection = quic->datagram_received_ev.connect(
      [this](const transport::packet::Endpoint &endpoint,
             const std::vector<uint8_t> &payload) {
        peer_router->ingest_receive(ripple::peer::TransportKind::quic,
                                    ripple::peer::ReceiveKind::datagram,
                                    endpoint, payload);
      });

  quic_transport_stream_receive_connection = quic->stream_received_ev.connect(
      [this](const transport::packet::Endpoint &endpoint,
             const std::vector<uint8_t> &payload) {
        peer_router->ingest_receive(ripple::peer::TransportKind::quic,
                                    ripple::peer::ReceiveKind::stream, endpoint,
                                    payload);
      });

  mcast_receive_connection = mcast->rx_signal.connect(
      [this](const transport::packet::RemotePacket &packet) {
        if (!packet.packet.data) {
          return;
        }
        peer_router->ingest_receive(ripple::peer::TransportKind::multicast,
                                    ripple::peer::ReceiveKind::packet,
                                    packet.endpoint, *packet.packet.data);
      });
};

Node::~Node() {
  peer_added_connection.disconnect();
  quic_state_connection.disconnect();
  quic_receive_connection.disconnect();
  quic_transport_receive_connection.disconnect();
  quic_transport_stream_receive_connection.disconnect();
  mcast_receive_connection.disconnect();

  io_context->stop();

  // wait for io_context to stop
  if (context_thread && context_thread->joinable())
    context_thread->join();
};

void Node::thread_loop() { io_context->run(); };

void Node::peer_added_handler(const peer_ptr peer) {
  logger->info("Handling new peer {}", peer->endpoints.at(0).to_string());

  peer_router->upsert_peer(peer->hash, peer->name, peer->endpoints.at(0));

  // add a client
  quic_client->add_endpoint(peer->endpoints.at(0));
};

void Node::quic_connection_state_handler(
    const transport::packet::Endpoint &endpoint, bool connected) {
  if (connected) {
    peer_router->mark_endpoint_active(endpoint);
    return;
  }

  peer_router->mark_endpoint_inactive(endpoint);
}

size_t Node::known_peer_count() const { return peer_manager->count(); }

} // namespace ripple::discovery