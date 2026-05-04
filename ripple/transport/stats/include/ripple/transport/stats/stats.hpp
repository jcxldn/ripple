
#ifndef STATS_STATS_HPP_
#define STATS_STATS_HPP_

#include <cstdint>

namespace ripple::transport::stats {

struct NetworkStats {
  uint64_t rtt_us = 0;            // Round trip time in microseconds
  uint64_t congestion_window = 0; // Congestion window in bytes
  uint64_t bandwidth = 0;         // Bandwidth estimate in bps
  uint64_t bytes_in_flight = 0;   // Bytes in flight
};

}; // namespace ripple::transport::stats

#endif /* STATS_STATS_HPP_ */