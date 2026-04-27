#ifndef PEER_ROUTER_HPP_
#define PEER_ROUTER_HPP_

#include "ripple/transport/packet/endpoint.hpp"

#include <boost/signals2.hpp>

#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ripple::peer {

enum class TransportKind {
  quic,
  multicast,
};

enum class ReceiveKind {
  datagram,
  stream,
  packet,
};

struct PeerRecord {
  std::string hash;
  std::string name;
  std::vector<transport::packet::Endpoint> endpoints;
  bool active = true;
};

struct ReceivedMessage {
  TransportKind transport;
  ReceiveKind kind;
  transport::packet::Endpoint endpoint;
  std::vector<uint8_t> payload;
};

class Router {
private:
  struct TransportKindHash {
    size_t operator()(const TransportKind kind) const {
      return static_cast<size_t>(kind);
    }
  };

  using Sender =
      std::function<bool(const PeerRecord &, const std::vector<uint8_t> &)>;

  mutable std::mutex mutex;
  std::unordered_map<std::string, PeerRecord> peers;
  std::unordered_map<TransportKind, Sender, TransportKindHash> senders;

public:
  boost::signals2::signal<void(const ReceivedMessage &)> rx_signal;

  boost::signals2::signal<void(const transport::packet::Endpoint &)>
      endpoint_added;

  void register_sender(TransportKind kind, Sender sender);

  void ingest_receive(TransportKind kind, ReceiveKind receive_kind,
                      const transport::packet::Endpoint &endpoint,
                      const std::vector<uint8_t> &payload);

  void upsert_peer(const std::string &hash, const std::string &name,
                   const transport::packet::Endpoint &endpoint);

  bool mark_peer_active(const std::string &hash);
  bool mark_peer_inactive(const std::string &hash);
  bool mark_endpoint_active(const transport::packet::Endpoint &endpoint);
  bool mark_endpoint_inactive(const transport::packet::Endpoint &endpoint);
  bool remove_peer(const std::string &hash);
  bool is_peer_active(const std::string &hash) const;

  bool send_to_peer(const std::string &hash,
                    const std::vector<uint8_t> &payload, TransportKind kind);

  size_t send_to_all_active(const std::vector<uint8_t> &payload,
                            TransportKind kind);

  size_t count() const;
};

} // namespace ripple::peer

#endif /* PEER_ROUTER_HPP_ */
