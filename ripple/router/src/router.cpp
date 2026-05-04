#include "ripple/peer/router.hpp"

#include <algorithm>

namespace ripple::peer {

void Router::ingest_receive(TransportKind kind, ReceiveKind receive_kind,
                            const transport::packet::Endpoint &endpoint,
                            const std::vector<uint8_t> &payload) {
  ReceivedMessage msg;
  msg.transport = kind;
  msg.kind = receive_kind;
  msg.endpoint = endpoint;
  msg.payload = payload;
  rx_signal(msg);
}

void Router::register_sender(TransportKind kind, Sender sender) {
  std::lock_guard<std::mutex> guard(mutex);
  senders[kind] = std::move(sender);
}

void Router::upsert_peer(const std::string &hash, const std::string &name,
                         const transport::packet::Endpoint &endpoint) {
  std::lock_guard<std::mutex> guard(mutex);

  auto it = peers.find(hash);
  if (it == peers.end()) {
    PeerRecord record;
    record.hash = hash;
    record.name = name;
    record.endpoints.push_back(endpoint);
    // emit an event so quic (if connection added) can connect to this endpoint
    endpoint_added(endpoint);
    peers.emplace(hash, std::move(record));
    return;
  }

  PeerRecord &record = it->second;
  // Any fresh peer signal is treated as liveness and reactivates the peer.
  record.active = true;
  if (!name.empty()) {
    record.name = name;
  }

  auto endpoint_it =
      std::find_if(record.endpoints.begin(), record.endpoints.end(),
                   [&endpoint](const transport::packet::Endpoint &current) {
                     return current.address == endpoint.address &&
                            current.port == endpoint.port;
                   });

  if (endpoint_it == record.endpoints.end()) {
    record.endpoints.push_back(endpoint);
    // emit an event so quic (if connection added) can connect to this endpoint
    endpoint_added(endpoint);
  }
}

bool Router::mark_peer_active(const std::string &hash) {
  std::lock_guard<std::mutex> guard(mutex);
  auto it = peers.find(hash);
  if (it == peers.end()) {
    return false;
  }
  it->second.active = true;
  return true;
}

bool Router::mark_peer_inactive(const std::string &hash) {
  std::lock_guard<std::mutex> guard(mutex);
  auto it = peers.find(hash);
  if (it == peers.end()) {
    return false;
  }
  it->second.active = false;
  return true;
}

bool Router::mark_endpoint_active(const transport::packet::Endpoint &endpoint) {
  std::lock_guard<std::mutex> guard(mutex);
  for (auto &[_, peer] : peers) {
    auto endpoint_it =
        std::find_if(peer.endpoints.begin(), peer.endpoints.end(),
                     [&endpoint](const transport::packet::Endpoint &current) {
                       return current.address == endpoint.address &&
                              current.port == endpoint.port;
                     });
    if (endpoint_it != peer.endpoints.end()) {
      peer.active = true;
      return true;
    }
  }
  return false;
}

bool Router::mark_endpoint_inactive(
    const transport::packet::Endpoint &endpoint) {
  std::lock_guard<std::mutex> guard(mutex);
  for (auto &[_, peer] : peers) {
    auto endpoint_it =
        std::find_if(peer.endpoints.begin(), peer.endpoints.end(),
                     [&endpoint](const transport::packet::Endpoint &current) {
                       return current.address == endpoint.address &&
                              current.port == endpoint.port;
                     });
    if (endpoint_it != peer.endpoints.end()) {
      peer.active = false;
      return true;
    }
  }
  return false;
}

bool Router::remove_peer(const std::string &hash) {
  std::lock_guard<std::mutex> guard(mutex);
  return peers.erase(hash) > 0;
}

bool Router::is_peer_active(const std::string &hash) const {
  std::lock_guard<std::mutex> guard(mutex);
  auto it = peers.find(hash);
  if (it == peers.end()) {
    return false;
  }
  return it->second.active;
}

bool Router::send_to_peer(const std::string &hash,
                          const std::vector<uint8_t> &payload,
                          TransportKind kind) {
  Sender sender;
  PeerRecord peer;
  {
    std::lock_guard<std::mutex> guard(mutex);

    auto sender_it = senders.find(kind);
    if (sender_it == senders.end()) {
      return false;
    }
    sender = sender_it->second;

    auto peer_it = peers.find(hash);
    if (peer_it == peers.end() || !peer_it->second.active) {
      return false;
    }
    peer = peer_it->second;
  }

  return sender(peer, payload);
}

size_t Router::send_to_all_active(const std::vector<uint8_t> &payload,
                                  TransportKind kind) {
  Sender sender;
  std::vector<PeerRecord> active_peers;
  {
    std::lock_guard<std::mutex> guard(mutex);

    auto sender_it = senders.find(kind);
    if (sender_it == senders.end()) {
      return 0;
    }
    sender = sender_it->second;

    active_peers.reserve(peers.size());
    for (const auto &entry : peers) {
      if (entry.second.active) {
        active_peers.push_back(entry.second);
      }
    }
  }

  size_t sent = 0;
  for (const auto &peer : active_peers) {
    if (sender(peer, payload)) {
      ++sent;
    }
  }

  return sent;
}

size_t Router::count() const {
  std::lock_guard<std::mutex> guard(mutex);
  return peers.size();
}

std::string Router::find_peer_hash_by_endpoint(
    const transport::packet::Endpoint &endpoint) const {
  std::lock_guard<std::mutex> guard(mutex);
  for (const auto &[hash, peer] : peers) {
    auto endpoint_it =
        std::find_if(peer.endpoints.begin(), peer.endpoints.end(),
                     [&endpoint](const transport::packet::Endpoint &current) {
                       return current.address == endpoint.address &&
                              current.port == endpoint.port;
                     });
    if (endpoint_it != peer.endpoints.end()) {
      return hash;
    }
  }
  return "";
}

std::string Router::find_peer_hash_by_name(const std::string &name) const {
  std::lock_guard<std::mutex> guard(mutex);
  for (const auto &[hash, peer] : peers) {
    if (peer.name == name) {
      return hash;
    }
  }
  return "";
}

void Router::update_network_stats(const transport::packet::Endpoint &endpoint,
                                  const transport::stats::NetworkStats &stats) {
  std::lock_guard<std::mutex> guard(mutex);
  stats_by_endpoint[endpoint.to_string()] = stats;
}

transport::stats::NetworkStats
Router::get_network_stats(const transport::packet::Endpoint &endpoint) const {
  std::lock_guard<std::mutex> guard(mutex);
  auto it = stats_by_endpoint.find(endpoint.to_string());
  if (it != stats_by_endpoint.end()) {
    return it->second;
  }
  return transport::stats::NetworkStats();
}

} // namespace ripple::peer
