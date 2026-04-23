#ifndef DISCOVERY_DISCO_PACKET_HPP_
#define DISCOVERY_DISCO_PACKET_HPP_

#include <boost/asio/buffer.hpp>
#include <string>
namespace ripple::discovery {

struct DiscoPacket {
  std::string protocol = "ripple";

  template <class Archive> void serialize(Archive &ar) { ar(protocol); }
};

} // namespace ripple::discovery

#endif /* DISCOVERY_DISCO_PACKET_HPP_ */