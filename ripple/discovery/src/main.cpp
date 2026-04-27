
#include "ripple/discovery/node.hpp"
#include "ripple/logger/logger.hpp"
#include "spdlog/common.h"

std::atomic<bool> keep_running(true);

void signal_handler(int signal) {
  if (signal == SIGINT) {
    keep_running = false;
  }
}

boost::asio::io_context io_context;
boost::asio::steady_timer act_timer(io_context);

std::chrono::milliseconds interval = std::chrono::milliseconds(2500);

void callback(ripple::discovery::Node *node) {
  auto l = ripple::logger::LoggerProvider::get_logger("main");
  auto pm = node->get_peer_manager();

  auto qc = node->get_quic_client();
  pm->for_each_active([&l, &qc](const ripple::discovery::peer_ptr &peer) {
    l->info("active peer: {}", peer->name);

    auto ep = peer->endpoints.at(0);

    std::string msg = "hello";

    std::vector<uint8_t> myVector(msg.begin(), msg.end());
    qc->send_datagram(ep, myVector);
  });

  act_timer.expires_after(interval);
  act_timer.async_wait(std::bind(callback, node));
};

int main(int argc, char **argv) {

  ripple::logger::LoggerProvider::set_level(spdlog::level::level_enum::debug);

  // start
  ripple::discovery::Node *node = new ripple::discovery::Node();

  // start loop to emulate traffic
  callback(node);

  std::thread th([]() { io_context.run(); });
  th.detach();

  // wait for ctrl c
  std::signal(SIGINT, signal_handler);
  while (keep_running) {
  };

  io_context.stop();

  // stop
  delete node;

  return 0;
};