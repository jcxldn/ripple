#include "ripple/transport/packet/controller.hpp"
#include "ripple/logger/logger.hpp"
#include "ripple/transport/packet/endpoint.hpp"
#include <functional>

namespace ripple::transport::packet {

PacketController::PacketController(
    std::shared_ptr<boost::asio::io_context> io_context) {
  this->io_context = io_context;

  logger = logger::LoggerProvider::get_logger(
      "ripple::transport::packet::controller");

  // Setup packet handling
  packets = std::make_shared<PacketMap>();
};

void PacketController::connect(
    boost::signals2::signal<void(const RemotePacket)> &sig) {
  // TODO: will this disconnect on controller destructor?
  sig.connect(boost::bind(&PacketController::packet_receive_handler, this,
                          std::placeholders::_1));
};

void PacketController::packet_receive_handler(const RemotePacket rp) {

  auto pkt = rp.packet;

  auto msg_default = std::make_shared<Message>();
  auto result = packets->emplace(pkt.header.msg_id, msg_default);
  bool is_new_message = result.second;
  std::shared_ptr<Message> msg = result.first->second;

  auto expire_callback = [logger = logger, id = pkt.header.msg_id, msg,
                          packets =
                              packets](const boost::system::error_code &e) {
    if (e != boost::asio::error::operation_aborted) {
      const std::lock_guard<std::mutex> lock(msg->m);

      logger->warn("Message id '{}' expired, removing...", id);

      packets->erase(id);
    }
  };

  if (msg.get() != nullptr) // shared_ptr has value
  {
    msg->m.lock();

    msg->receive(pkt, io_context, expire_callback);
    msg->m.unlock();

    if (msg->payload.size() > 0) {
      // message avail.
      logger->trace("Received complete message id '{}' with total size '{}'",
                    pkt.header.msg_id, msg->payload.size());

      msg->expire_timer->cancel();

      msg->source = std::move(rp.endpoint);

      rx_signal(msg);
    }
  }
};

} // namespace ripple::transport::packet