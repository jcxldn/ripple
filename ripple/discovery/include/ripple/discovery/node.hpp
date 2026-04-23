#ifndef DISCOVERY_NODE_HPP_
#define DISCOVERY_NODE_HPP_

#include "ripple/logger/logger.hpp"
#include "ripple/util/cert/identity.hpp"

namespace ripple::discovery {

class Node {
private:
  std::shared_ptr<logger::logger> logger;

  util::cert::id_ptr id;

public:
  Node();
};

} // namespace ripple::discovery

#endif /* DISCOVERY_NODE_HPP_ */