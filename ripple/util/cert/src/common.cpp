#include "ripple/util/cert/common.hpp"
#include "openssl/bio.h"
#include "openssl/crypto.h"
#include "openssl/x509v3.h"
#include <boost/beast/core/detail/base64.hpp>
#include <stdexcept>

using namespace boost::beast::detail;

namespace ripple::util::cert {
// nid: numeric identifier
void add_extension(cert_ptr &cert, int nid, const char *value) {
  X509V3_CTX ctx;

  // Set extension context without a configuration database
  X509V3_set_ctx_nodb(&ctx);

  // Assuming self signed certs, issuer and subject are both target, no request
  // or crl
  X509V3_set_ctx(&ctx, cert.get(), cert.get(), nullptr, nullptr, 0);

  // Create the extension
  X509_EXTENSION *ex =
      X509V3_EXT_conf_nid(nullptr, &ctx, nid, const_cast<char *>(value));

  if (!ex)
    throw std::runtime_error(std::string("ext ") + value + " failed");

  // Add to certificate
  X509_add_ext(cert.get(), ex, -1);

  // Free temp extension object
  X509_EXTENSION_free(ex);
};

std::string serialize_bio(bio_ptr &bio) {
  char *data = nullptr;
  long len = BIO_get_mem_data(bio.get(), &data);
  return std::string(data, data + len);
};

std::string cert_to_pem(cert_ptr &cert) {
  bio_ptr bio(BIO_new(BIO_s_mem()));
  if (!PEM_write_bio_X509(bio.get(), cert.get()))
    throw std::runtime_error("PEM_write_bio_X509 failed");
  return serialize_bio(bio);
};

std::string key_to_pem(key_ptr &key) {
  bio_ptr bio(BIO_new(BIO_s_mem()));
  if (!PEM_write_bio_PrivateKey(bio.get(), key.get(), nullptr, nullptr, 0,
                                nullptr, nullptr))
    throw std::runtime_error("PEM_write_bio_PrivateKey failed");
  return serialize_bio(bio);
};

key_ptr key_from_pem(const std::string &pem) {

  bio_ptr bio(BIO_new_mem_buf(pem.data(), pem.size()));
  EVP_PKEY *raw = PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr);
  if (!raw)
    throw std::runtime_error("PEM_read_bio_PrivateKey failed");
  return key_ptr(raw);
};

cert_ptr cert_from_pem(const std::string &pem) {
  bio_ptr bio(BIO_new_mem_buf(pem.data(), pem.size()));
  X509 *raw = PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr);
  if (!raw)
    throw std::runtime_error("PEM_read_bio_X509 failed");
  return cert_ptr(raw);
};

std::vector<uint8_t> hash_cert_spki(cert_ptr &cert) {
  unsigned char *der = nullptr;
  int len = i2d_X509_PUBKEY(X509_get_X509_PUBKEY(cert.get()), &der);
  if (len <= 0)
    throw std::runtime_error("i2d_X509_PUBKEY failed");
  std::vector<uint8_t> out(32);
  unsigned int olen = 0;
  EVP_Digest(der, len, out.data(), &olen, EVP_sha256(), nullptr);
  OPENSSL_free(der);
  return out;
};

std::string spki_hash_to_b64(const std::vector<uint8_t> &hash) {
  std::string dest;
  dest.resize(base64::encoded_size(hash.size()));
  std::size_t written =
      base64::encode(&dest[0], hash.data(), hash.size());
  dest.resize(written);
  return dest;
};

} // namespace ripple::util::cert
