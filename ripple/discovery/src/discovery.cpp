
#include "ripple/discovery/discovery.hpp"
#include "ripple/discovery/peer.hpp"
#include "ripple/transport/multicast/mcast.hpp"

#include "ripple/serialization/cereal.hpp"

#include "ripple/discovery/disco_packet.hpp"
#include "ripple/transport/packet/packet.hpp"
#include <functional>
#include <memory>

namespace ripple::discovery {

DiscoveryNode::DiscoveryNode(
    int target_port, util::cert::id_ptr identity,
    std::shared_ptr<boost::asio::io_context> io_context,
    std::shared_ptr<transport::multicast::MulticastTransport> transport,
    std::shared_ptr<transport::packet::PacketController> controller,
    std::shared_ptr<PeerManager> manager) {
  this->target_port = target_port;
  this->identity = identity;
  this->transport = transport;
  this->controller = controller;
  this->manager = manager;

  logger =
      logger::LoggerProvider::get_logger("ripple::discovery::DiscoveryNode");

  // Note the program start time
  start_millis = get_epoch_millis();

  // create a timer to transmit a packet
  disco_timer = new boost::asio::steady_timer(*io_context.get());
  disco_interval = std::chrono::milliseconds(DISCO_INTERVAL_MS);

  // start timer
  disco_timer_act();

  // register message listener
  controller->rx_signal.connect(
      boost::bind(&DiscoveryNode::net_callback, this, std::placeholders::_1));
};

long DiscoveryNode::get_epoch_millis() {

  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  long millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  return millis;
}

std::shared_ptr<ripple::discovery::DiscoPacket> DiscoveryNode::create_pkt() {
  std::shared_ptr<ripple::discovery::DiscoPacket> pkt =
      std::make_shared<ripple::discovery::DiscoPacket>();

  pkt->name = identity->get_cn();
  pkt->port = target_port;
  pkt->hash = identity->spki_b64();
  pkt->millis = get_epoch_millis() - start_millis;

  return pkt;
};

void DiscoveryNode::transmit_pkt() {
  auto pkt = create_pkt();

  // serialize packet
  auto payload = ripple::serialization::serialize(pkt);

  // encode the packet in a message
  auto msg = std::make_shared<ripple::transport::packet::Message>();
  msg->encode(payload->buf);

  // send each packet in the message.
  for (auto &pkt_entry : msg->pkts) {
    ripple::transport::packet::Packet p = pkt_entry.second;
    transport->transmit(p.serialize());
  }
};

void DiscoveryNode::disco_timer_act() {
  transmit_pkt();

  disco_timer->expires_after(disco_interval);
  disco_timer->async_wait(boost::bind(&DiscoveryNode::disco_timer_act, this));
}

void DiscoveryNode::net_callback(
    const std::shared_ptr<transport::packet::Message> msg) {
  msg->m.lock();

  // assume message is of type Disco
  auto pkt = serialization::deserialize<DiscoPacket>(msg->payload);

  // check to ensure this packet wasn't from us (diff hash)
  if (identity->spki_b64().compare(pkt->hash) != 0) {

    // check to see if this peer is already known to us
    if (!manager->is_hash_known(pkt->hash)) {
      // new peer!
      logger->warn("Found new peer: {}, {}", pkt->name, pkt->hash);

      // construct a new peer object
      auto peer = std::make_shared<Peer>();
      update_peer_from_disco(peer, pkt);

      manager->add_peer(peer);
    } else {
      // known peer, update
      auto peer = manager->get_peer_by_hash(pkt->hash);
      update_peer_from_disco(peer, pkt);
    }
  }

  msg->m.unlock();
};

void DiscoveryNode::update_peer_from_disco(
    peer_ptr peer, std::shared_ptr<ripple::discovery::DiscoPacket> pkt) {
  // update name if required
  if (peer->name.compare(pkt->name) != 0) {
    peer->name = pkt->name;
  }

  if (peer->hash.compare(pkt->hash) != 0) {
    peer->hash = pkt->hash;
  }

  // update endpoints (skip)

  // update last epoch
  // todo check if old was higher? edge case possibly
  peer->last_remote_epoch = pkt->millis;
};

} // namespace ripple::discovery