#ifndef CONNECTION_QUIC_HPP_
#define CONNECTION_QUIC_HPP_

#include "ripple/logger/logger.hpp"
#include "ripple/peer/peer.hpp"
#include "ripple/transport/quic/client.hpp"
#include "ripple/transport/quic/quic.hpp"

namespace ripple::peer::connection {

class QuicConnection {
private:
  std::shared_ptr<logger::logger> logger;

  Peer *peer;

  std::shared_ptr<ripple::transport::quic::QuicTransport> quic;
  std::shared_ptr<ripple::transport::quic::QuicClient> quic_client;

  boost::signals2::scoped_connection quic_state_connection;
  boost::signals2::scoped_connection quic_receive_connection;
  boost::signals2::scoped_connection quic_transport_receive_connection;
  boost::signals2::scoped_connection quic_transport_stream_receive_connection;

  void
  quic_connection_state_handler(const transport::packet::Endpoint &endpoint,
                                bool connected);

public:
  QuicConnection(Peer *peer);
  ~QuicConnection();

  inline int16_t get_port() { return quic->get_port(); };
};

} // namespace ripple::peer::connection

#endif /* CONNECTION_QUIC_HPP_ */
