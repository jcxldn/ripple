#ifndef PACKET_PACKET_HPP_
#define PACKET_PACKET_HPP_

#include "ripple/transport/packet/endpoint.hpp"
#include <array>
#include <boost/asio.hpp>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>

namespace ripple::transport::packet {

typedef std::array<char, 1024> data_array;

typedef uint32_t MessageId;

static uint16_t MAGIC = 0xF00D;

struct Header {
  uint16_t magic = MAGIC;
  uint32_t msg_id;
  uint32_t chunk_id;
  uint32_t chunk_count;
};

struct Packet {
  Header header;
  std::shared_ptr<std::vector<uint8_t>> data;

  std::shared_ptr<std::vector<uint8_t>> serialize() const;

  static Packet deserialize(boost::asio::const_buffer buf);
};

class Message {
private:
  static const int MAX_MSG_SIZE = 1280;
  static const int MAX_PAYLOAD_SIZE = 1000;

  std::shared_ptr<std::vector<uint8_t>>
      payload_storage; // asio buffers don't own their data (just a ref)

public:
  Message() = default;

  void
  receive(Packet pkt, std::shared_ptr<boost::asio::io_context> io_context,
          std::function<void(const boost::system::error_code)> expire_callback);

  void encode(boost::asio::mutable_buffer payload);

  uint32_t max_chunks;

  std::shared_ptr<boost::asio::steady_timer> expire_timer;

  Endpoint source;

  std::mutex m;

  std::map<uint32_t, Packet> pkts;
  boost::asio::const_buffer payload;
};
} // namespace ripple::transport::packet

#endif /* PACKET_PACKET_HPP_ */