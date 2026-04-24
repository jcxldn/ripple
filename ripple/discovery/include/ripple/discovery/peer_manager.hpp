#ifndef DISCOVERY_PEER_MANAGER_HPP_
#define DISCOVERY_PEER_MANAGER_HPP_

#include "ripple/discovery/peer.hpp"

namespace ripple::discovery {

class PeerManager {
private:
  std::vector<peer_ptr> peers;

public:
  peer_ptr get_peer_by_hash(std::string &hash);
  bool is_hash_known(std::string &hash);

  void add_peer(peer_ptr peer);
};

}; // namespace ripple::discovery

#endif /* DISCOVERY_PEER_MANAGER_HPP_ */