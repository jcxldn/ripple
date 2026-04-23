#include "ripple/util/cert/ed25519.hpp"
#include "ripple/util/cert/cert.hpp"
#include "ripple/util/cert/common.hpp"

#include <openssl/evp.h>
#include <stdexcept>

namespace ripple::util::cert::ed25519 {

key_ptr generate_key() {
  pkey_ctx_ptr ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr));
  if (!ctx)
    throw std::runtime_error("EVP_PKEY_CTX_new_id failed");
  if (EVP_PKEY_keygen_init(ctx.get()) <= 0)
    throw std::runtime_error("keygen_init failed");
  EVP_PKEY *raw = nullptr;
  if (EVP_PKEY_keygen(ctx.get(), &raw) <= 0)
    throw std::runtime_error("keygen failed");
  return key_ptr(raw);
};

} // namespace ripple::util::cert::ed25519