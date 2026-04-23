#ifndef MULTICAST_TX_PAIR_HPP_
#define MULTICAST_TX_PAIR_HPP_

#include <memory>

#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include "ripple/logger/logger.hpp"

namespace ripple::transport::multicast {

class TransmitPair {
private:
  std::shared_ptr<logger::logger> logger;

  std::shared_ptr<boost::asio::ip::udp::endpoint> endpoint;
  std::shared_ptr<boost::asio::ip::udp::socket> socket;

public:
  TransmitPair(std::shared_ptr<boost::asio::io_context> io_context,
               boost::asio::ip::address mcast_ip, boost::asio::ip::address ip,
               int port);
  ~TransmitPair();

  std::size_t send(boost::asio::mutable_buffer &buf);
};

} // namespace ripple::transport::multicast

#endif /* MULTICAST_TX_PAIR_HPP_*/