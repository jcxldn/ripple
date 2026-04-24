#ifndef PACKET_CONTROLLER_HPP_
#define PACKET_CONTROLLER_HPP_

#include "ripple/logger/logger.hpp"
#include "ripple/transport/packet/packet.hpp"
#include "ripple/transport/packet/remote_packet.hpp"
#include <boost/signals2.hpp>

namespace ripple::transport::packet {

typedef std::unordered_map<MessageId, std::shared_ptr<Message>> PacketMap;

class PacketController {
private:
  std::shared_ptr<logger::logger> logger;

  std::shared_ptr<PacketMap> packets;

  // io context to run eg. expire callbacks in
  std::shared_ptr<boost::asio::io_context> io_context;

  void packet_receive_handler(const RemotePacket rp);

public:
  PacketController(std::shared_ptr<boost::asio::io_context> io_context);
  //~PacketController();

  void connect(boost::signals2::signal<void(const RemotePacket)> &sig);

  boost::signals2::signal<void(const std::shared_ptr<Message>)> rx_signal;
};

} // namespace ripple::transport::packet
#endif /* PACKET_CONTROLLER_HPP_ */