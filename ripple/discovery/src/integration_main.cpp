#include "ripple/discovery/node.hpp"

#include <chrono>
#include <thread>

int main() {
  ripple::discovery::Node node_a;
  ripple::discovery::Node node_b;

  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(10);

  while (std::chrono::steady_clock::now() < deadline) {
    if (node_a.known_peer_count() > 0 && node_b.known_peer_count() > 0) {
      return 0;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 1;
}