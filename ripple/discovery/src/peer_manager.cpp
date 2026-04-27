#include "ripple/discovery/peer_manager.hpp"
#include "ripple/discovery/peer.hpp"
#include <algorithm>

namespace ripple::discovery {

peer_ptr PeerManager::get_peer_by_hash(std::string &hash) {
  auto it = std::find_if(peers.begin(), peers.end(), [hash](const peer_ptr &p) {
    return p->hash.compare(hash) == 0;
  });

  if (it != peers.end()) {
    // found
    return *it;
  } else {
    return std::shared_ptr<Peer>(); // empty
  }
}

bool PeerManager::is_hash_known(std::string &hash) {
  return (bool)get_peer_by_hash(hash);
};

void PeerManager::add_peer(peer_ptr peer) {
  // TODO: check if already exists

  peers.push_back(peer);

  peer_added_ev(peer);
}
} // namespace ripple::discovery