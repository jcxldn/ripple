#ifndef DISCOVERY_PEER_HPP_
#define DISCOVERY_PEER_HPP_

#include "ripple/transport/packet/endpoint.hpp"
#include <algorithm>
#include <string>
namespace ripple::discovery {

struct Peer {
  std::string name;
  std::string hash;

  std::vector<transport::packet::Endpoint>
      endpoints; // rx'd ip and quic port from pkt

  // last rx'd 'time since boot' from other node
  long last_remote_epoch;

  inline bool is_endpoint_known(transport::packet::Endpoint endpoint) {
    auto it =
        std::find_if(endpoints.begin(), endpoints.end(),
                     [&endpoint](const transport::packet::Endpoint current) {
                       bool portMatch = (endpoint.port == current.port);
                       bool ipMatch = (endpoint.address.to_string().compare(
                                           current.address.to_string()) == 0);
                       return (portMatch && ipMatch);
                     });

    return it != endpoints.end();
  };
};

using peer_ptr = std::shared_ptr<Peer>;

}; // namespace ripple::discovery

#endif /* DISCOVERY_PEER_HPP_ */