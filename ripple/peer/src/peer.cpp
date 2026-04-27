#include "ripple/peer/peer.hpp"
#include "ripple/util/cert/identity.hpp"
#include <memory>

namespace ripple::peer {

Peer::Peer() {
  logger = logger::LoggerProvider::get_logger("ripple::peer::Peer");

  logger->info("Initializing Ripple peer");

  // Create a router
  router = std::make_shared<Router>();

  // Generate a random (default)identity
};

void Peer::new_identity() {
  logger->info(
      "Ephemeral identity in use, consider setting a persistent identity.");
  id = std::make_shared<util::cert::Identity>();
};

void Peer::start() {
  // check if we have an identity set (such as a persistent one)
  if (!id)
    new_identity();

  logger->info("Using identity '{}' (hash {})", id->get_cn(), id->spki_b64());
};

} // namespace ripple::peer