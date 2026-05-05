
#include "ripple/logger/logger.hpp"
#include "ripple/peer/connection/discovery.hpp"
#include "ripple/peer/connection/quic.hpp"
#include "ripple/peer/peer.hpp"
#include "ripple/peer/router.hpp"

std::atomic<bool> keep_running(true);

void signal_handler(int signal) {
  if (signal == SIGINT) {
    keep_running = false;
  }
}

int main(int argc, char **argv) {
  ripple::logger::LoggerProvider::set_level(spdlog::level::level_enum::trace);

  auto peer = new ripple::peer::Peer();

  // use default identity
  // peer.set_identity(...)

  peer->start();

  // create QUIC (reliable - streams) connections
  auto qc = new ripple::peer::connection::QuicConnection(peer);

  // create Discovery (over unreliable multicast)
  auto dc =
      new ripple::peer::connection::DiscoveryConnection(peer, qc->get_port());

  // dbg: logger
  auto l = ripple::logger::LoggerProvider::get_logger("demo");

  peer->get_router()->rx_signal.connect(
      [&l](const ripple::peer::ReceivedMessage &rx) {
        l->warn("Received {} bytes from {}", rx.payload.size(),
                rx.endpoint.to_string());
        std::string dat(rx.payload.begin(), rx.payload.end());
        l->warn("Payload: '{}'", dat);
      });

  std::this_thread::sleep_for(std::chrono::milliseconds(2500));

  std::string msg = "hello";
  std::vector<uint8_t> bytes(msg.begin(), msg.end());

  peer->get_router()->send_to_all_active(bytes,
                                         ripple::peer::TransportKind::quic);

  std::signal(SIGINT, signal_handler);
  while (keep_running) {
  };

  delete dc;
  delete qc;
  delete peer;

  return 0;
};