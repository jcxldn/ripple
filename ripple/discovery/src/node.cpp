#include "ripple/discovery/node.hpp"
#include "ripple/util/cert/identity.hpp"
#include <memory>

namespace ripple::discovery {

Node::Node() {

  logger = logger::LoggerProvider::get_logger("ripple::discovery::Node");

  // Generate an identity (cert / key self signed pair)
  id = std::make_shared<util::cert::Identity>();

  logger->info("Cert hash: {}", id->spki_b64());
};

} // namespace ripple::discovery