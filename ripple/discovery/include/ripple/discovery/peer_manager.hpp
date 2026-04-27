#ifndef DISCOVERY_PEER_MANAGER_HPP_
#define DISCOVERY_PEER_MANAGER_HPP_

#include "ripple/discovery/peer.hpp"
#include <boost/signals2.hpp>
#include <mutex>

namespace ripple::discovery {

class PeerManager {
private:
  std::vector<peer_ptr> peers;
  mutable std::mutex peers_mutex;

public:
  peer_ptr get_peer_by_hash(std::string &hash);
  bool is_hash_known(std::string &hash);

  void add_peer(peer_ptr peer);
  size_t count() const;

  boost::signals2::signal<void(const peer_ptr peer)> peer_added_ev;

  void for_each_active(std::function<void(const peer_ptr)> callback);
};

}; // namespace ripple::discovery

#endif /* DISCOVERY_PEER_MANAGER_HPP_ */