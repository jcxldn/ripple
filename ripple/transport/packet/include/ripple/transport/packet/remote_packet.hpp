#ifndef PACKET_REMOTE_PACKET_HPP_
#define PACKET_REMOTE_PACKET_HPP_

#include "ripple/transport/packet/endpoint.hpp"
#include "ripple/transport/packet/packet.hpp"

namespace ripple::transport::packet {

struct RemotePacket {
  Endpoint endpoint;
  Packet packet;
};

} // namespace ripple::transport::packet

#endif /* PACKET_REMOTE_PACKET_HPP_ */