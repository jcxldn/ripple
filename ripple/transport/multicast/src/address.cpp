#include "ripple/transport/multicast/address.hpp"

#include <ifaddrs.h>

namespace ripple::transport::multicast {

std::vector<boost::asio::ip::address> get_local_addresses_v4() {
  std::vector<boost::asio::ip::address> addrs;

  struct ifaddrs *ifaddr;
  if (getifaddrs(&ifaddr) == -1)
    return addrs;

  for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr)
      continue;
    if (!(ifa->ifa_flags & IFF_UP))
      continue;
    if (ifa->ifa_addr->sa_family == AF_INET) {
      struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
      addrs.push_back(
          boost::asio::ip::make_address_v4(ntohl(sa->sin_addr.s_addr)));
    }
    // Add IPv6 handling...
  }
  freeifaddrs(ifaddr);
  return addrs;
};

std::vector<boost::asio::ip::address>
get_local_addresses_v4_in_network(boost::asio::ip::network_v4 const &network) {
  std::vector<boost::asio::ip::address> addrs_all = get_local_addresses_v4();

  std::vector<boost::asio::ip::address> addrs;

  for (boost::asio::ip::address &addr : addrs_all) {
    if (network.canonical().address() ==
        boost::asio::ip::network_v4(addr.to_v4(), network.prefix_length())
            .canonical()
            .address()) {
      addrs.push_back(addr);
    }
  }

  return addrs;
};

} // namespace ripple::transport::multicast
