
#include "ripple/discovery/discovery.hpp"
#include "ripple/transport/multicast/mcast.hpp"

#include "ripple/serialization/cereal.hpp"

#include "ripple/discovery/disco_packet.hpp"
#include "ripple/transport/packet/packet.hpp"
#include <functional>

namespace ripple::discovery {

DiscoveryNode::DiscoveryNode(
    int target_port, util::cert::id_ptr identity,
    std::shared_ptr<boost::asio::io_context> io_context,
    std::shared_ptr<transport::multicast::MulticastTransport> transport) {
  this->target_port = target_port;
  this->identity = identity;
  this->transport = transport;

  // Note the program start time
  start_millis = get_epoch_millis();

  // create a timer to transmit a packet
  disco_timer = new boost::asio::steady_timer(*io_context.get());
  disco_interval = std::chrono::milliseconds(DISCO_INTERVAL_MS);

  // start timer
  disco_timer_act();
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
  disco_timer->async_wait(std::bind(&DiscoveryNode::disco_timer_act, this));
}

} // namespace ripple::discovery