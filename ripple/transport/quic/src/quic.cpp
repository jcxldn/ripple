#include "ripple/transport/quic/quic.hpp"
#include "msquic.h"
#include "ripple/transport/quic/api.hpp"
#include "ripple/util/cert/common.hpp"
#include <algorithm>
#include <chrono>
#include <memory>
#include <netinet/in.h>

namespace ripple::transport::quic {

namespace {

transport::packet::Endpoint
endpoint_from_quic_addr(const QuicAddr &remote_addr) {
  transport::packet::Endpoint remote_endpoint{};
  remote_endpoint.port = remote_addr.GetPort();

  if (remote_addr.GetFamily() == QUIC_ADDRESS_FAMILY_INET) {
    remote_endpoint.address = boost::asio::ip::address_v4(
        ntohl(remote_addr.SockAddr.Ipv4.sin_addr.s_addr));
  } else if (remote_addr.GetFamily() == QUIC_ADDRESS_FAMILY_INET6) {
    boost::asio::ip::address_v6::bytes_type bytes{};
    const auto *raw = remote_addr.SockAddr.Ipv6.sin6_addr.s6_addr;
    std::copy(raw, raw + bytes.size(), bytes.begin());
    remote_endpoint.address = boost::asio::ip::address_v6(
        bytes, remote_addr.SockAddr.Ipv6.sin6_scope_id);
  }

  return remote_endpoint;
}

} // namespace

QuicTransport::QuicTransport(QuicOptions &opt, util::cert::id_ptr identity) {
  this->opt = opt;
  this->identity = identity;

  logger = logger::LoggerProvider::get_logger("ripple::transport::quic");

  logger->info("Starting QUIC transport");

  bool success = protocol_init();

  if (success) {
    logger->info("QUIC transport ready.");
  }
};

QuicTransport::~QuicTransport() {
  {
    std::lock_guard<std::mutex> guard(active_connections_mutex);
    shutting_down = true;
  }

  listener.reset();

  if (registration) {
    registration->Shutdown(QUIC_CONNECTION_SHUTDOWN_FLAG_SILENT, 1);

    std::unique_lock<std::mutex> guard(active_connections_mutex);
    active_connections_drained.wait_for(guard, std::chrono::seconds(1), [this] {
      return active_connections.empty();
    });
  }

  configuration.reset();
  registration.reset();

  api.reset();
}

bool QuicTransport::protocol_init() {
  api = acquire_msquic_api();
  if (!api) {
    logger->critical("MsQuicApi init failed!");
    return false;
  }

  // Register (defaults to low latency but explicitly set here for ease of use)
  registration = std::make_unique<MsQuicRegistration>(
      opt.protocol_name.c_str(), QUIC_EXECUTION_PROFILE_LOW_LATENCY);
  if (!registration->IsValid()) {
    logger->critical("created MsQuicRegistration was not valid.");
    return false;
  }

  // settings
  MsQuicSettings settings;
  settings.SetIdleTimeoutMs(opt.idle_timeout_ms)
      .SetKeepAlive(opt.keep_alive_ms)
      .SetPeerBidiStreamCount(opt.peer_stream_bidirectional_count)
      .SetPeerUnidiStreamCount(opt.peer_stream_unidirectional_count)
      .SetDatagramReceiveEnabled(DATAGRAM_RX_ENABLED)
      .SetServerResumptionLevel(RESUMPTION_LEVEL)
      .SetNetStatsEventEnabled(true);

  // Load credentials from identity
  QUIC_CREDENTIAL_CONFIG creds = init_create_cred_config();

  MsQuicAlpn alpn(opt.alpn.c_str());

  // create a quic config with all our opts
  configuration = std::make_unique<MsQuicConfiguration>(
      *registration, alpn, settings, MsQuicCredentialConfig(creds));
  if (!configuration->IsValid()) {
    logger->critical("config load failed, check cert");
    return false;
  }

  // create listener
  listener = std::make_unique<MsQuicAutoAcceptListener>(
      *registration, *configuration, quic_conn_callback, this);

  // random port
  addr = QuicAddr(QUIC_ADDRESS_FAMILY_UNSPEC);

  if (QUIC_FAILED(listener->Start(alpn, &addr.SockAddr))) {
    logger->critical("Listener could not start.");
    return false;
  }

  logger->info("listening on udp *:{}, with alpn '{}'", get_port(), opt.alpn);

  return true;
};

QUIC_CREDENTIAL_CONFIG QuicTransport::init_create_cred_config() {
  // convert identity keypair into pkcs12 blob (in mem)
  identity_pkcs12_blob = identity->pkcs12_blob();

  identity_pkcs12.Asn1Blob = identity_pkcs12_blob.data();
  identity_pkcs12.Asn1BlobLength =
      static_cast<uint32_t>(identity_pkcs12_blob.size());
  identity_pkcs12.PrivateKeyPassword = nullptr;

  // create cred config struct
  QUIC_CREDENTIAL_CONFIG cred{};
  cred.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_PKCS12;
  cred.CertificatePkcs12 = &identity_pkcs12;
  cred.Flags = QUIC_CREDENTIAL_FLAG_REQUIRE_CLIENT_AUTHENTICATION |
               QUIC_CREDENTIAL_FLAG_INDICATE_CERTIFICATE_RECEIVED |
               QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION; // server

  // return ref
  return cred;
};

uint16_t QuicTransport::get_port() {
  QuicAddr addr;
  listener->GetLocalAddr(addr);
  uint16_t port = addr.GetPort();
  return port;
};

QUIC_STATUS QUIC_API QuicTransport::quic_conn_callback(
    MsQuicConnection *conn, void *ctx, QUIC_CONNECTION_EVENT *ev) {
  auto *qt = static_cast<QuicTransport *>(ctx);

  // Track from the very first event so connections that abort during the
  // handshake (before CONNECTED) are still visible to the drain wait in the
  // destructor. The unordered_set insertion is idempotent on repeat events.
  if (ev->Type != QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE) {
    std::lock_guard<std::mutex> guard(qt->active_connections_mutex);
    qt->active_connections.insert(conn);
  }

  switch (ev->Type) {
  case QUIC_CONNECTION_EVENT_CONNECTED:
    if (!qt->shutting_down) {
      qt->logger->info("[conn {}]: connected", static_cast<void *>(conn));
    }
    break;
  case QUIC_CONNECTION_EVENT_PEER_CERTIFICATE_RECEIVED: {
    auto *raw_cert =
        static_cast<X509 *>(ev->PEER_CERTIFICATE_RECEIVED.Certificate);
    if (!raw_cert) {
      qt->logger->critical("[conn {}] no peer cert", static_cast<void *>(conn));
      return QUIC_STATUS_BAD_CERTIFICATE;
    }
    util::cert::cert_ptr peer_cert(X509_dup(raw_cert));
    if (!peer_cert) {
      qt->logger->critical("[conn {}] X509_dup failed",
                           static_cast<void *>(conn));
      return QUIC_STATUS_BAD_CERTIFICATE;
    }
    auto hash = util::cert::spki_hash_to_b64(util::cert::hash_cert_spki(peer_cert));
    qt->logger->info("[conn {}] client SPKI: {}", static_cast<void *>(conn),
                     hash);
    std::lock_guard<std::mutex> guard(qt->conn_hash_mutex);
    qt->conn_to_peer_hash[conn] = std::move(hash);
    break;
  }
  case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED: {
    auto *stream_ctx = new StreamCallbackContext();
    stream_ctx->transport = qt;

    QuicAddr remote_addr;
    if (QUIC_SUCCEEDED(conn->GetRemoteAddr(remote_addr))) {
      stream_ctx->remote_endpoint = endpoint_from_quic_addr(remote_addr);
    }
    {
      std::lock_guard<std::mutex> guard(qt->conn_hash_mutex);
      auto it = qt->conn_to_peer_hash.find(conn);
      if (it != qt->conn_to_peer_hash.end()) {
        stream_ctx->peer_hash = it->second;
      }
    }

    // Create a stream object which is auto cleaned up
    // on shutdown with our callback
    new MsQuicStream(ev->PEER_STREAM_STARTED.Stream, CleanUpAutoDelete,
                     quic_stream_callback, stream_ctx);
    break;
  }
  case QUIC_CONNECTION_EVENT_DATAGRAM_RECEIVED: {
    const auto *b = ev->DATAGRAM_RECEIVED.Buffer;
    std::vector<uint8_t> payload;
    if (b && b->Buffer && b->Length > 0) {
      payload.assign(b->Buffer, b->Buffer + b->Length);
    }

    transport::packet::Endpoint remote_endpoint{};
    QuicAddr remote_addr;
    if (QUIC_SUCCEEDED(conn->GetRemoteAddr(remote_addr))) {
      remote_endpoint = endpoint_from_quic_addr(remote_addr);
    }

    std::string peer_hash;
    {
      std::lock_guard<std::mutex> guard(qt->conn_hash_mutex);
      auto it = qt->conn_to_peer_hash.find(conn);
      if (it != qt->conn_to_peer_hash.end()) {
        peer_hash = it->second;
      }
    }

    qt->logger->info("[conn {}] rx datagram {} bytes", (void *)conn, b->Length);
    qt->datagram_received_ev(remote_endpoint, peer_hash, payload);
    break;
  }
  case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
    qt->logger->warn("[conn {}]: shutdown by transport, status=0x{:x} error={}",
                     (void *)conn, ev->SHUTDOWN_INITIATED_BY_TRANSPORT.Status,
                     ev->SHUTDOWN_INITIATED_BY_TRANSPORT.ErrorCode);
    break;
  case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
    qt->logger->warn("[conn {}]: shutdown by peer, error={}", (void *)conn,
                     ev->SHUTDOWN_INITIATED_BY_PEER.ErrorCode);
    break;
  case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE: {
    {
      std::lock_guard<std::mutex> guard(qt->active_connections_mutex);
      qt->active_connections.erase(conn);
    }
    {
      std::lock_guard<std::mutex> guard(qt->conn_hash_mutex);
      qt->conn_to_peer_hash.erase(conn);
    }
    qt->active_connections_drained.notify_all();
  } break;
  case QUIC_CONNECTION_EVENT_NETWORK_STATISTICS: {
    auto stats = ev->NETWORK_STATISTICS;

    // Create NetworkStats struct from MsQuic statistics
    transport::stats::NetworkStats network_stats{};
    network_stats.rtt_us = stats.SmoothedRTT;
    network_stats.congestion_window = stats.CongestionWindow;
    network_stats.bandwidth = stats.Bandwidth;
    network_stats.bytes_in_flight = stats.BytesInFlight;

    // Get remote endpoint and emit signal
    transport::packet::Endpoint remote_endpoint{};
    QuicAddr remote_addr;
    if (QUIC_SUCCEEDED(conn->GetRemoteAddr(remote_addr))) {
      remote_endpoint = endpoint_from_quic_addr(remote_addr);
    }

    qt->logger->debug("[conn {}] network stats: rtt={}us, cwnd={}, bw={} bps",
                      (void *)conn, network_stats.rtt_us,
                      network_stats.congestion_window, network_stats.bandwidth);

    qt->network_stats_ev(remote_endpoint, network_stats);
    break;
  }
  default:
    break;
  }
  return QUIC_STATUS_SUCCESS;
};

QUIC_STATUS QUIC_API QuicTransport::quic_stream_callback(
    MsQuicStream *stream, void *ctx, QUIC_STREAM_EVENT *ev) {
  auto *stream_ctx = static_cast<StreamCallbackContext *>(ctx);
  if (!stream_ctx || !stream_ctx->transport) {
    return QUIC_STATUS_SUCCESS;
  }

  auto *qt = stream_ctx->transport;

  switch (ev->Type) {
  case QUIC_STREAM_EVENT_RECEIVE: {
    uint64_t total = ev->RECEIVE.TotalBufferLength;
    std::vector<uint8_t> payload;
    payload.reserve(static_cast<size_t>(total));

    for (uint32_t i = 0; i < ev->RECEIVE.BufferCount; ++i) {
      const auto &buffer = ev->RECEIVE.Buffers[i];
      if (!buffer.Buffer || buffer.Length == 0) {
        continue;
      }
      payload.insert(payload.end(), buffer.Buffer,
                     buffer.Buffer + buffer.Length);
    }

    qt->logger->info("[stream {}] rx {} bytes", static_cast<void *>(stream),
                     total);
    qt->stream_received_ev(stream_ctx->remote_endpoint, stream_ctx->peer_hash,
                           payload);
    // Return SUCCESS to signal all bytes consumed; MsQuic will not deliver
    // them again. Use QUIC_STATUS_PENDING + StreamReceiveComplete() for async.
    // -> return QUIC_STATUS_SUCCESS when data consumed!
    break;
  }
  case QUIC_STREAM_EVENT_SEND_COMPLETE:
    // Send buffer returned to caller; Canceled=TRUE means it was not sent.
    if (ev->SEND_COMPLETE.Canceled) {
      qt->logger->warn("[stream {}] send canceled",
                       static_cast<void *>(stream));
    }
    break;
  case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
    // Peer sent FIN — graceful half-close of their send side.
    qt->logger->debug("[stream {}] peer send shutdown",
                      static_cast<void *>(stream));
    break;
  case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
    qt->logger->warn("[stream {}] peer send aborted, error={}",
                     static_cast<void *>(stream),
                     ev->PEER_SEND_ABORTED.ErrorCode);
    break;
  case QUIC_STREAM_EVENT_PEER_RECEIVE_ABORTED:
    qt->logger->warn("[stream {}] peer receive aborted, error={}",
                     static_cast<void *>(stream),
                     ev->PEER_RECEIVE_ABORTED.ErrorCode);
    break;
  case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
    // Stream is fully torn down. CleanUpAutoDelete frees the MsQuicStream.
    qt->logger->debug("[stream {}] shutdown complete",
                      static_cast<void *>(stream));
    delete stream_ctx;
    break;
  default:
    break;
  }
  return QUIC_STATUS_SUCCESS;
}
} // namespace ripple::transport::quic