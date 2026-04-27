#ifndef PACKET_ENDPOINT_HPP_
#define PACKET_ENDPOINT_HPP_

#include <boost/asio/ip/address.hpp>
#include <string>

namespace ripple::transport::packet {

struct Endpoint {
  boost::asio::ip::address address;
  uint16_t port;

  inline std::string to_string() const {
    return address.to_string() + ":" + std::to_string(port);
  }

  inline bool operator==(const Endpoint &other) const {
    return port == other.port && address == other.address;
  }
};

} // namespace ripple::transport::packet

#endif /* PACKET_ENDPOINT_HPP_ */