#include "ripple/util/cert/identity.hpp"
#include "openssl/pkcs12.h"
#include "ripple/util/cert/cert.hpp"
#include "ripple/util/cert/common.hpp"

#include "ripple/util/cert/ed25519.hpp"
#include <cstddef>

#include <boost/beast/core/detail/base64.hpp>

using namespace boost::beast::detail;

namespace ripple::util::cert {

Identity::Identity(const std::string &cn, const std::string &san,
                   int validity_days) {

  this->cn = cn;
  this->san = san;

  // Create an ed25519 key
  key_ptr key = ed25519::generate_key();

  // Create a certificate using the generated key
  cert_ptr cert = create_self_signed_cert(key, cn, san, validity_days);

  // construct an identity
  cert_pem = cert_to_pem(cert);
  key_pem = key_to_pem(key);
  spki_sha256 = hash_cert_spki(cert);
};

std::vector<uint8_t> Identity::pkcs12_blob() {
  // get cert and key ptrs
  key_ptr key = key_from_pem(key_pem);
  cert_ptr cert = cert_from_pem(cert_pem);

  // Create a pkcs12 ptr
  pkcs12_ptr p12(
      PKCS12_create(nullptr,             // password
                    cn.c_str(),          // friendly name
                    key.get(),           // priv key
                    cert.get(),          // cert
                    nullptr,             // ca
                    0,                   // key cipher (default)
                    0,                   // cert cipher (default),
                    PKCS12_DEFAULT_ITER, // iteration
                    1, // mac iter - insecure as too low (ephemeral pair though)
                    0  // keytype
                    ));

  if (!p12)
    throw std::runtime_error("PKCS12_create failed");

  // serialize in mem
  bio_ptr bio(BIO_new(BIO_s_mem()));
  if (i2d_PKCS12_bio(bio.get(), p12.get()) <= 0)
    throw std::runtime_error("i2d_PKCS12_bio failed");

  BUF_MEM *mem = nullptr;
  BIO_get_mem_ptr(bio.get(), &mem);
  return std::vector<uint8_t>(reinterpret_cast<uint8_t *>(mem->data),
                              reinterpret_cast<uint8_t *>(mem->data) +
                                  mem->length);
};

std::string Identity::spki_b64() {
  std::size_t input_size = spki_sha256.size() * sizeof(uint8_t);

  // create a std::string of the correct size to hold the encoded data
  std::string dest;
  dest.resize(base64::encoded_size(input_size));

  // reinterpret vector data as raw bytes and encode
  auto const *src = reinterpret_cast<const std::uint8_t *>(spki_sha256.data());
  std::size_t written = base64::encode(&dest[0], src, input_size);

  // Ensure the string is correctly sized (base64::encode returns written chars)
  dest.resize(written);
  return dest;
};

}; // namespace ripple::util::cert