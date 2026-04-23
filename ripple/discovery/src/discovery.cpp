
#include "ripple/transport/multicast/mcast.hpp"
#include "ripple/transport/packet/packet.hpp"

#include "ripple/serialization/cereal.hpp"

#include "ripple/discovery/disco_packet.hpp"
int main(int argc, char **argv) {
  ripple::transport::multicast::MulticastOptions mcast_opt;
  ripple::transport::multicast::MulticastTransport mcast_tp(mcast_opt);

  auto msg = std::make_shared<ripple::transport::packet::Message>();

  std::shared_ptr<ripple::discovery::DiscoPacket> pkt =
      std::make_shared<ripple::discovery::DiscoPacket>();

  auto payload = ripple::serialization::serialize(pkt);

  msg->encode(payload->buf);

  // send over mcast

  for (auto &pkt_entry : msg->pkts) {
    ripple::transport::packet::Packet p = pkt_entry.second;
    mcast_tp.transmit(p.serialize());
  }
};