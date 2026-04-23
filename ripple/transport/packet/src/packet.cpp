#include "ripple/transport/packet/packet.hpp"

#include "ripple/util/prng.hpp"
#include <boost/endian/conversion.hpp>
#include <cmath>
#include <cstring>
#include <iostream>

namespace ripple::transport::packet {

std::shared_ptr<std::vector<uint8_t>> Packet::serialize() const {
  const size_t header_size = sizeof(Header);
  const size_t payload_size = data ? data->size() : 0;
  const size_t pkt_size = header_size + payload_size;

  auto pkt_data = std::make_shared<std::vector<uint8_t>>(pkt_size);

  Header wire_header = header;
  boost::endian::native_to_big_inplace(wire_header.magic);
  boost::endian::native_to_big_inplace(wire_header.msg_id);
  boost::endian::native_to_big_inplace(wire_header.chunk_id);
  boost::endian::native_to_big_inplace(wire_header.chunk_count);

  // copy header
  std::memcpy(pkt_data->data(), &wire_header, header_size);

  // copy payload
  if (payload_size > 0) {
    std::memcpy(pkt_data->data() + header_size, data->data(), payload_size);
  }

  return pkt_data;
};
Packet Packet::deserialize(boost::asio::const_buffer buf) {
  Packet p;

  const size_t header_size = sizeof(Header);
  if (buf.size() < header_size) {
    std::cout << "deserialize: buffer too small for header (" << buf.size()
              << ")\n";
    return p;
  }

  // copy header from start
  Header wire_header{};
  std::memcpy(&wire_header, buf.data(), header_size);

  boost::endian::native_to_big_inplace(wire_header.magic);
  boost::endian::big_to_native_inplace(wire_header.msg_id);
  boost::endian::big_to_native_inplace(wire_header.chunk_id);
  boost::endian::big_to_native_inplace(wire_header.chunk_count);

  p.header = wire_header;

  // copy payload (if present)
  const size_t payload_size = buf.size() - header_size;
  if (payload_size > 0) {
    p.data = std::make_shared<std::vector<uint8_t>>(payload_size);
    const auto *payload_start =
        static_cast<const uint8_t *>(buf.data()) + header_size;
    std::memcpy(p.data->data(), payload_start, payload_size);
  }

  return p;
};

void Message::receive(
    Packet pkt, std::shared_ptr<boost::asio::io_context> io_context,
    std::function<void(const boost::system::error_code)> expire_callback) {
  if (pkts.find(pkt.header.chunk_id) == pkts.end()) {
    // new chunk found

    pkts.insert({pkt.header.chunk_id, pkt});
  }

  // New packet found this loop
  if (pkts.size() == 1) {
    max_chunks = pkt.header.chunk_count;

    // create expire timer
    expire_timer = std::make_shared<boost::asio::steady_timer>(
        *io_context.get(), boost::asio::chrono::seconds(10));
    expire_timer->async_wait(expire_callback);
  }

  // If completed
  if (pkts.size() == max_chunks) {
    // Re-assemble

    // determine size
    size_t size = 0;
    for (auto &pkt_entry : pkts) {
      size += pkt_entry.second.data->size();
    }

    // reconstruct
    auto rebuilt = std::make_shared<std::vector<uint8_t>>(size);

    // copy
    for (auto &pkt_entry : pkts) {
      auto pkt = pkt_entry.second;
      uint32_t start = int(pkt.header.chunk_id * MAX_PAYLOAD_SIZE);
      std::memcpy(rebuilt->data() + start, pkt.data->data(), pkt.data->size());
    }

    payload_storage = rebuilt;
    payload = boost::asio::buffer(*payload_storage);
  }
};

void Message::encode(boost::asio::mutable_buffer payload) {
  this->payload = payload;

  // make one side of divide float for float div
  float required_chunks_float = payload.size() / (float)MAX_PAYLOAD_SIZE;
  int required_chunks = std::ceil(required_chunks_float);

  // make pseudorandom msg id
  util::PRNG p;
  uint32_t msg_id = p.random_u32();

  uint32_t chunk_size = MAX_PAYLOAD_SIZE;

  size_t last_index =
      payload.size(); // exclusive (aka +1 after end since zero indexed)

  // get payload contents
  char *data = static_cast<char *>(payload.data());

  for (int chunk = 0; chunk < required_chunks; chunk++) {
    Packet pkt;
    pkt.header.chunk_count = required_chunks;
    pkt.header.chunk_id = chunk;
    pkt.header.msg_id = msg_id;

    // chunk size
    uint32_t start = int(chunk * chunk_size); // prob min/clamp this too?
    uint32_t end = std::min(size_t(start + chunk_size),
                            last_index); // end point (exclusive)

    uint32_t size = end - start;

    // buffer to store the data in
    pkt.data = std::make_shared<std::vector<uint8_t>>(size);
    // copy ranged data into buffer
    std::copy(data + start, data + end,
              pkt.data->begin()); // start inclusive, right exclusive

    pkts.insert({pkt.header.chunk_id, pkt});
  }
};
} // namespace ripple::transport::packet
