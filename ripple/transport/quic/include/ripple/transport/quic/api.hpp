#ifndef QUIC_API_HPP_
#define QUIC_API_HPP_

#define QUIC_API_ENABLE_PREVIEW_FEATURES 1
#include <msquic.hpp>

#include <memory>

namespace ripple::transport::quic {

std::shared_ptr<MsQuicApi>
acquire_msquic_api(std::shared_ptr<MsQuicApi> preferred = nullptr);

} // namespace ripple::transport::quic

#endif /* QUIC_API_HPP_ */