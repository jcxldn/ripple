#ifndef CERT_COMMON_HPP_
#define CERT_COMMON_HPP_

#include "openssl/pkcs12.h"
#include <memory>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <string>
#include <vector>

namespace ripple::util::cert {

struct bio_free {
  void operator()(BIO *b) const { BIO_free(b); }
};
struct key_free {
  void operator()(EVP_PKEY *p) const { EVP_PKEY_free(p); }
};
struct cert_free {
  void operator()(X509 *x) const { X509_free(x); }
};
struct pkey_ctx_Free {
  void operator()(EVP_PKEY_CTX *c) const { EVP_PKEY_CTX_free(c); }
};

struct pkcs12_free {
  void operator()(PKCS12 *p) const { PKCS12_free(p); }
};

using bio_ptr = std::unique_ptr<BIO, bio_free>;
using key_ptr = std::unique_ptr<EVP_PKEY, key_free>;
using cert_ptr = std::unique_ptr<X509, cert_free>;
using pkey_ctx_ptr = std::unique_ptr<EVP_PKEY_CTX, pkey_ctx_Free>;
using pkcs12_ptr = std::unique_ptr<PKCS12, pkcs12_free>;

void add_extension(cert_ptr &cert, int nid, const char *value);

std::string serialize_bio(bio_ptr *&bio);

std::string cert_to_pem(cert_ptr &cert);
std::string key_to_pem(key_ptr &key);

key_ptr key_from_pem(const std::string &pem);
cert_ptr cert_from_pem(const std::string &pem);

std::vector<uint8_t> hash_cert_spki(cert_ptr &cert);
std::vector<uint8_t> hash_cert_spki(X509 *cert);
std::string spki_hash_to_b64(const std::vector<uint8_t> &hash);

} // namespace ripple::util::cert

#endif /* CERT_COMMON_HPP_ */