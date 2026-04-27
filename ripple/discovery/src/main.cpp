
#include "ripple/discovery/node.hpp"
#include "ripple/logger/logger.hpp"
#include "spdlog/common.h"

std::atomic<bool> keep_running(true);

void signal_handler(int signal) {
  if (signal == SIGINT) {
    keep_running = false;
  }
}

int main(int argc, char **argv) {

  ripple::logger::LoggerProvider::set_level(spdlog::level::level_enum::debug);

  // start
  ripple::discovery::Node *node = new ripple::discovery::Node();

  // wait for ctrl c
  std::signal(SIGINT, signal_handler);
  while (keep_running) {
  };

  // stop
  delete node;

  return 0;
};