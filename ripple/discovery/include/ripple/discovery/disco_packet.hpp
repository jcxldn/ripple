#ifndef DISCOVERY_DISCO_PACKET_HPP_
#define DISCOVERY_DISCO_PACKET_HPP_

#include <boost/asio/buffer.hpp>
#include <string>
namespace ripple::discovery {

struct DiscoPacket {
  std::string protocol = "ripple";

  int port; // quic listening port

  std::string name;
  std::string hash; // hash of pubkey for quic server

  long millis; // millis since program start

  template <class Archive> void serialize(Archive &ar) {
    ar(protocol, port, name, hash, millis);
  }
};

} // namespace ripple::discovery

#endif /* DISCOVERY_DISCO_PACKET_HPP_ */