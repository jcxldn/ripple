#ifndef NET_ADDRESS_HPP_
#define NET_ADDRESS_HPP_

#include <boost/asio.hpp>
#include <vector>

namespace ripple::transport::multicast {

struct cidr_v4 {
  std::string ip = "0.0.0.0";
  int mask = 0;

  inline boost::asio::ip::network_v4 network() {
    return boost::asio::ip::network_v4(
        {boost::asio::ip::make_address_v4(ip), (short unsigned int)mask});
  }
};

std::vector<boost::asio::ip::address> get_local_addresses_v4();

std::vector<boost::asio::ip::address>
get_local_addresses_v4_in_network(boost::asio::ip::network_v4 const &network);

} // namespace ripple::transport::multicast

#endif /* NET_ADDRESS_HPP_*/