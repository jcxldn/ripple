

#ifndef QUIC_OPTIONS_HPP_
#define QUIC_OPTIONS_HPP_

#include <cstdint>
#include <string>
namespace ripple::transport::quic {

struct QuicOptions {
  std::string protocol_name = "ripple-quic";
  std::string alpn = "ripple";
  uint64_t idle_timeout_ms = 5000;
  uint32_t keep_alive_ms = 1000;
  uint16_t peer_stream_bidirectional_count = 100;
  uint16_t peer_stream_unidirectional_count = 100;
};
} // namespace ripple::transport::quic

#endif /* QUIC_OPTIONS_HPP_*/