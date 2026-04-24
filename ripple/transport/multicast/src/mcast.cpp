#include "ripple/transport/multicast/mcast.hpp"
#include "ripple/logger/logger.hpp"
#include "ripple/transport/packet/endpoint.hpp"
#include <memory>

namespace ripple::transport::multicast {

MulticastTransport::MulticastTransport(MulticastOptions &opt) {
  this->opt = opt;

  logger = logger::LoggerProvider::get_logger("ripple::transport::multicast");

  mcast_ip = ::boost::asio::ip::make_address(opt.address);

  logger->info("Starting multicast for {}:{}", opt.address, opt.port);

  logger->info("Targeting {}/{}", opt.listen_interface.ip,
               opt.listen_interface.mask);

  io_context = std::make_shared<boost::asio::io_context>();

  local_addresses =
      get_local_addresses_v4_in_network(opt.listen_interface.network());

  rx_setup();
  tx_setup();

  context_thread =
      std::make_shared<std::thread>(&MulticastTransport::thread_loop, this);
};

MulticastTransport::~MulticastTransport() {
  rx_stop_signal.emit(boost::asio::cancellation_type::terminal);
  io_context->stop();

  // wait for io_context to stop
  if (context_thread && context_thread->joinable())
    context_thread->join();

  // close threads if open
  if (rx_socket && rx_socket->is_open())
    rx_socket->close();

  // close each tx socket
  tx_pairs.clear();
}

void MulticastTransport::transmit(
    const std::shared_ptr<std::vector<uint8_t>> msg) {
  tx_signal(std::move(msg));
}

void MulticastTransport::tx_setup() {
  // setup sockets
  for (auto &local_addr : local_addresses) {
    if (local_addr.is_loopback() && !opt.enable_loopback) {
      logger->debug("Skipping loopback IP '{}'", local_addr.to_string());
      continue;
    }

    logger->info("Creating socket on interface with address {}",
                 local_addr.to_string());

    std::shared_ptr<TransmitPair> pair = std::make_shared<TransmitPair>(
        io_context, mcast_ip, local_addr, opt.port);

    tx_pairs.push_back(pair);
  }

  // connect tx action

  tx_signal.connect(
      boost::bind(&MulticastTransport::tx_action, this, std::placeholders::_1));
};

void MulticastTransport::tx_action(
    const std::shared_ptr<std::vector<uint8_t>> bytes) {
  auto buf = boost::asio::buffer(bytes->data(), bytes->size());

  // send on each socket (one per subscribed interface)
  for (std::shared_ptr<TransmitPair> &pair : tx_pairs) {
    pair->send(buf);
  }
};

void MulticastTransport::rx_setup() {
  rx_stop_state = boost::asio::cancellation_state(
      rx_stop_signal.slot()); // Binds internally

  // setup socket
  boost::asio::ip::address addr =
      boost::asio::ip::make_address(opt.listen_interface.ip);
  rx_endpoint =
      std::make_shared<boost::asio::ip::udp::endpoint>(addr, opt.port);
  rx_socket = std::make_shared<boost::asio::ip::udp::socket>(*io_context.get());
  rx_socket->open(rx_endpoint->protocol());
  // rx_socket->non_blocking(true);
  rx_socket->set_option(boost::asio::ip::udp::socket::reuse_address(true));
  rx_socket->bind(*rx_endpoint.get());
  // join mcast group
  boost::asio::ip::address mcast_addr =
      boost::asio::ip::make_address(opt.address);

  for (auto &local_addr : local_addresses) {
    if (local_addr.is_loopback() && !opt.enable_loopback) {
      logger->debug("Skipping loopback IP '{}'", local_addr.to_string());
      continue;
    }

    logger->info("Setting up multicast RX on interface with address {}",
                 local_addr.to_string());
    rx_socket->set_option(boost::asio::ip::multicast::join_group(
        mcast_addr.to_v4(), local_addr.to_v4()));
  }

  auto data = std::make_shared<std::array<uint8_t, 1024>>();
  auto sender_endpoint = std::make_shared<boost::asio::ip::udp::endpoint>();

  auto do_receive = std::make_shared<std::function<void()>>();
  *do_receive = [this, data, sender_endpoint, do_receive]() {
    rx_socket->async_receive_from(
        boost::asio::buffer(*data), *sender_endpoint,

        boost::asio::bind_cancellation_slot(
            rx_stop_signal.slot(),
            [this, data, sender_endpoint,
             do_receive](boost::system::error_code err_code, size_t length) {
              if (length > 0 && !err_code) {
                boost::asio::ip::address src_address =
                    sender_endpoint->address();
                bool is_local_address =
                    std::find(local_addresses.begin(), local_addresses.end(),
                              src_address) != local_addresses.end();

                if (!opt.enable_loopback &&
                    (src_address.is_loopback() || is_local_address)) {
                  // discard loopback packet as loopback rx is not enabled
                } else {
                  boost::asio::const_buffer buf(data->data(), length);

                  packet::RemotePacket rp;

                  rp.packet = packet::Packet::deserialize(buf);

                  if (rp.packet.data->size() > 0 &&
                      rp.packet.header.magic == packet::MAGIC) {
                    rp.endpoint.address = sender_endpoint->address();
                    rp.endpoint.port = sender_endpoint->port();

                    rx_signal(rp);
                  }
                }
              }
              (*do_receive)();
            }));
  };

  // arm first receive
  (*do_receive)();
};

void MulticastTransport::thread_loop() {
  // boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
  // guard(io_context.get_executor());
  io_context->run();
};

} // namespace ripple::transport::multicast