#ifndef DISCOVERY_DISCO_PACKET_HPP_
#define DISCOVERY_DISCO_PACKET_HPP_

#include <boost/asio/buffer.hpp>
#include <string>
namespace ripple::discovery {

struct DiscoPacket {
  std::string protocol = "ripple";

  int port;         // quic listening port
  std::string hash; // hash of pubkey for quic server

  template <class Archive> void serialize(Archive &ar) { ar(protocol); }
};

} // namespace ripple::discovery

#endif /* DISCOVERY_DISCO_PACKET_HPP_ */