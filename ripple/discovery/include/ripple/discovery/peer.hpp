#ifndef DISCOVERY_PEER_HPP_
#define DISCOVERY_PEER_HPP_

#include "ripple/transport/multicast/address.hpp"
#include <string>
namespace ripple::discovery {

struct Peer {
  std::string name;
  std::string hash;

  std::vector<transport::multicast::cidr_v4>
      endpoints; // rx'd ip and quic port from pkt

  // last rx'd 'time since boot' from other node
  long last_remote_epoch;
};

using peer_ptr = std::shared_ptr<Peer>;

}; // namespace ripple::discovery

#endif /* DISCOVERY_PEER_HPP_ */