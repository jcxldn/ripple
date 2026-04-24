#ifndef MULTICAST_MCAST_HPP_
#define MULTICAST_MCAST_HPP_

#include <memory>
#include <string>

#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include "ripple/logger/logger.hpp"
#include "ripple/transport/multicast/address.hpp"
#include "ripple/transport/multicast/tx_pair.hpp"
#include "ripple/transport/packet/packet.hpp"

namespace ripple::transport::multicast {

struct MulticastOptions {
  int ttl = 255;
  int port = 46785;
  std::string address = "224.0.0.176";
  bool enable_loopback = true;
  cidr_v4 listen_interface;
};

class MulticastTransport {

private:
  MulticastOptions opt;
  std::shared_ptr<logger::logger> logger;

  boost::asio::ip::address mcast_ip;

  // threads
  std::shared_ptr<std::thread> context_thread;

  // #region dual (asio)
  std::shared_ptr<boost::asio::io_context> io_context;
  // # endregion

  // graceful shutdown
  std::shared_ptr<boost::asio::signal_set> sigs;

  // #region transmit (asio)
  std::vector<std::shared_ptr<TransmitPair>>
      tx_pairs; // each subscribed interface has a socket

  void tx_setup();
  void tx_action(const std::shared_ptr<std::vector<uint8_t>> bytes);
  // #endregion

  // #region receive (asio)
  std::shared_ptr<boost::asio::ip::udp::endpoint> rx_endpoint;
  std::shared_ptr<boost::asio::ip::udp::socket> rx_socket;

  boost::asio::cancellation_signal rx_stop_signal;
  boost::asio::cancellation_state rx_stop_state;

  std::vector<boost::asio::ip::address> local_addresses;

  void rx_setup();
  //  #endregion

  void thread_loop();

public:
  MulticastTransport(MulticastOptions &opt);
  ~MulticastTransport();

  boost::signals2::signal<void(const std::shared_ptr<std::vector<uint8_t>>)>
      tx_signal;
  boost::signals2::signal<void(const packet::Packet)> rx_signal;

  void transmit(std::shared_ptr<std::vector<uint8_t>> msg);

  inline std::shared_ptr<boost::asio::io_context> get_io_context() {
    return io_context;
  };
};
} // namespace ripple::transport::multicast

#endif /* MULTICAST_MCAST_HPP_*/