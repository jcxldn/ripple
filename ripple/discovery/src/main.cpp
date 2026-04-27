
#include "ripple/discovery/node.hpp"
#include "ripple/logger/logger.hpp"
#include "ripple/peer/router.hpp"
#include "spdlog/common.h"
#include <boost/signals2/connection.hpp>

std::atomic<bool> keep_running(true);

void signal_handler(int signal) {
  if (signal == SIGINT) {
    keep_running = false;
  }
}

boost::asio::io_context io_context;
boost::asio::steady_timer act_timer(io_context);
boost::signals2::scoped_connection router_rx_connection;

std::chrono::milliseconds interval = std::chrono::milliseconds(2500);

void callback(ripple::discovery::Node *node) {
  auto l = ripple::logger::LoggerProvider::get_logger("main");
  auto router = node->get_peer_router();
  auto peer_manager = node->get_peer_manager();
  auto quic_client = node->get_quic_client();

  std::string msg = "hello";
  std::vector<uint8_t> bytes(msg.begin(), msg.end());

  size_t sent =
      router->send_to_all_active(bytes, ripple::peer::TransportKind::quic);
  l->info("broadcast sent to {} peers via quic", sent);

  std::string stream_msg = "hello-stream";
  std::vector<uint8_t> stream_bytes(stream_msg.begin(), stream_msg.end());
  size_t stream_sent = 0;
  peer_manager->for_each_active([&quic_client, &stream_bytes, &stream_sent](
                                    const ripple::discovery::peer_ptr &peer) {
    if (!peer || peer->endpoints.empty()) {
      return;
    }
    if (quic_client->send_stream(peer->endpoints.front(), stream_bytes)) {
      ++stream_sent;
    }
  });
  l->info("stream sent to {} peers via quic", stream_sent);

  act_timer.expires_after(interval);
  act_timer.async_wait(std::bind(callback, node));
};

int main(int argc, char **argv) {

  ripple::logger::LoggerProvider::set_level(spdlog::level::level_enum::debug);

  // start
  ripple::discovery::Node *node = new ripple::discovery::Node();

  router_rx_connection = node->get_peer_router()->rx_signal.connect(
      [](const ripple::peer::ReceivedMessage &msg) {
        auto l = ripple::logger::LoggerProvider::get_logger("main");
        const char *transport =
            msg.transport == ripple::peer::TransportKind::quic ? "quic"
                                                               : "multicast";
        const char *kind = "packet";
        if (msg.kind == ripple::peer::ReceiveKind::datagram) {
          kind = "datagram";
        } else if (msg.kind == ripple::peer::ReceiveKind::stream) {
          kind = "stream";
        }
        l->info("router rx [{}:{}] {} bytes from {}", transport, kind,
                msg.payload.size(), msg.endpoint.to_string());
      });

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