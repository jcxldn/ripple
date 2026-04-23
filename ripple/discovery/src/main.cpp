
#include "ripple/discovery/node.hpp"
int main(int argc, char **argv) {
  // start
  ripple::discovery::Node *node = new ripple::discovery::Node();

  // wait 5s
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));

  // stop
  delete node;

  return 0;
};