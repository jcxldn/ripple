#include "ripple/transport/multicast/tx_pair.hpp"
#include "ripple/logger/logger.hpp"

namespace ripple::transport::multicast {
TransmitPair::TransmitPair(std::shared_ptr<boost::asio::io_context> io_context,
                           boost::asio::ip::address mcast_ip,
                           boost::asio::ip::address ip, int port, int ttl) {
  logger = logger::LoggerProvider::get_logger(
      "ripple::transport::multicast::TransmitPair");

  endpoint = std::make_shared<boost::asio::ip::udp::endpoint>(mcast_ip, port);
  socket = std::make_shared<boost::asio::ip::udp::socket>(*io_context.get(),
                                                          endpoint->protocol());

  socket->set_option(
      boost::asio::ip::multicast::join_group(mcast_ip.to_v4(), ip.to_v4()));
  socket->set_option(
      boost::asio::ip::multicast::outbound_interface(ip.to_v4()));
  socket->set_option(boost::asio::ip::multicast::hops(ttl));
};

TransmitPair::~TransmitPair() {
  logger->trace("Cleaning up for address {}", endpoint->address().to_string());

  if (socket && socket->is_open())
    socket->close();

  socket.reset();
  endpoint.reset();
}

size_t TransmitPair::send(boost::asio::mutable_buffer &buf) {
  size_t sent_bytes = socket->send_to(buf, (*endpoint.get()));

  logger->trace("Sent {} bytes on interface IP {}", sent_bytes,
                endpoint->address().to_string());

  return sent_bytes;
}
} // namespace ripple::transport::multicast