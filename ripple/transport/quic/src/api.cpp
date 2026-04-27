#include "ripple/transport/quic/api.hpp"

#include "msquic.h"

#include <mutex>

namespace ripple::transport::quic {

namespace {

std::mutex g_msquic_api_mutex;
std::weak_ptr<MsQuicApi> g_shared_msquic_api;

std::shared_ptr<MsQuicApi> make_managed_msquic_api() {
  auto deleter = [](MsQuicApi *api) {
    std::lock_guard<std::mutex> guard(g_msquic_api_mutex);
    if (MsQuic == api) {
      MsQuic = nullptr;
    }
    delete api;
  };

  return std::shared_ptr<MsQuicApi>(new MsQuicApi(), deleter);
}

} // namespace

std::shared_ptr<MsQuicApi>
acquire_msquic_api(std::shared_ptr<MsQuicApi> preferred) {
  std::lock_guard<std::mutex> guard(g_msquic_api_mutex);

  if (auto existing = g_shared_msquic_api.lock()) {
    MsQuic = existing.get();
    return existing;
  }

  auto api = preferred ? std::move(preferred) : make_managed_msquic_api();
  if (!api || QUIC_FAILED(api->GetInitStatus())) {
    return nullptr;
  }

  MsQuic = api.get();
  g_shared_msquic_api = api;
  return api;
}

} // namespace ripple::transport::quic