#include "ripple/peer/connection/quic.hpp"
#include "ripple/peer/router.hpp"

namespace ripple::peer::connection {

QuicConnection::QuicConnection(Peer *peer) {
  this->peer = peer;

  logger = logger::LoggerProvider::get_logger("ripple::peer::connection::Quic");

  transport::quic::QuicOptions quic_options;

  // create quic server
  quic = std::make_shared<transport::quic::QuicTransport>(quic_options,
                                                          peer->get_identity());

  // create quic client
  quic_client = std::make_shared<transport::quic::QuicClient>(
      quic_options, peer->get_identity(), quic->get_api());

  // register quic client as a sender
  peer->get_router()->register_sender(
      ripple::peer::TransportKind::quic,
      [client = quic_client](const ripple::peer::PeerRecord &peer,
                             const std::vector<uint8_t> &payload) {
        if (peer.endpoints.empty()) {
          return false;
        }
        // reliable messaging (streams)
        return client->send_stream(peer.endpoints.front(), payload);
      });

  // quic client: listen for connection events and rx data
  quic_state_connection = quic_client->connection_state_ev.connect(
      boost::bind(&QuicConnection::quic_connection_state_handler, this,
                  std::placeholders::_1, std::placeholders::_2));

  // this is unused in curr impl, we use unidirectional from our server inst.
  quic_receive_connection = quic_client->datagram_received_ev.connect(
      [this](const transport::packet::Endpoint &endpoint,
             const std::vector<uint8_t> &payload) {
        this->peer->get_router()->ingest_receive(
            ripple::peer::TransportKind::quic,
            ripple::peer::ReceiveKind::datagram, endpoint, payload);
      });

  // quic server: listen for connection events
  quic_transport_receive_connection = quic->datagram_received_ev.connect(
      [this](const transport::packet::Endpoint &endpoint,
             const std::vector<uint8_t> &payload) {
        this->peer->get_router()->ingest_receive(
            ripple::peer::TransportKind::quic,
            ripple::peer::ReceiveKind::datagram, endpoint, payload);
      });

  quic_transport_stream_receive_connection = quic->stream_received_ev.connect(
      [this](const transport::packet::Endpoint &endpoint,
             const std::vector<uint8_t> &payload) {
        this->peer->get_router()->ingest_receive(
            ripple::peer::TransportKind::quic,
            ripple::peer::ReceiveKind::stream, endpoint, payload);
      });

  // subscribe to router new peer events
  peer->get_router()->endpoint_added.connect(
      [this](const transport::packet::Endpoint &endpoint) {
        quic_client->add_endpoint(endpoint);
      });
};

QuicConnection::~QuicConnection() {
  quic_state_connection.disconnect();
  quic_receive_connection.disconnect();
  quic_transport_receive_connection.disconnect();
  quic_transport_stream_receive_connection.disconnect();
}

void QuicConnection::quic_connection_state_handler(
    const transport::packet::Endpoint &endpoint, bool connected) {
  if (connected) {
    peer->get_router()->mark_endpoint_active(endpoint);
    return;
  }

  peer->get_router()->mark_endpoint_inactive(endpoint);
}

}; // namespace ripple::peer::connection