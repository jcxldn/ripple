#ifndef PEER_PEER_HPP_
#define PEER_PEER_HPP_

#include "ripple/logger/logger.hpp"
#include "ripple/peer/router.hpp"

#include "ripple/util/cert/identity.hpp"

#include <memory>

namespace ripple::peer {

class Peer {
private:
  std::shared_ptr<logger::logger> logger;

  std::shared_ptr<Router> router;

  util::cert::id_ptr id;

  void new_identity();

public:
  Peer();

  void start();

  void set_identity(util::cert::id_ptr &id);
  inline util::cert::id_ptr get_identity() { return id; };

  inline std::shared_ptr<Router> get_router() { return router; };
};

} // namespace ripple::peer

#endif /* PEER_PEER_HPP_ */
