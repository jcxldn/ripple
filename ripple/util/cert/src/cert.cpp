#include "ripple/util/cert/cert.hpp"
#include "openssl/asn1.h"
#include "openssl/obj_mac.h"
#include "openssl/x509.h"
#include "ripple/util/cert/common.hpp"
#include "ripple/util/prng.hpp"
#include <stdexcept>

namespace ripple::util::cert {

cert_ptr create_self_signed_cert(key_ptr &key, const std::string &cn,
                                 const std::string &san, int validity_days) {
  // Create the certificate object
  cert_ptr cert(X509_new());

  if (!cert)
    throw std::runtime_error("X509_new failed");

  // Set x509v3
  X509_set_version(cert.get(), X509_VERSION_3);

  // Get a random 64 bit serial
  ripple::util::PRNG prng;
  ASN1_INTEGER_set_uint64(X509_get_serialNumber(cert.get()), prng.random_u64());

  // Set validity from now to +1 year
  X509_gmtime_adj(X509_getm_notBefore(cert.get()), 0);
  X509_gmtime_adj(X509_getm_notAfter(cert.get()),
                  static_cast<long>(validity_days) * 24 * 3600);

  // Set public key
  X509_set_pubkey(cert.get(), key.get());

  // Get subject name to construct issuer name with
  X509_NAME *name = X509_get_subject_name(cert.get());

  // Add common name field
  X509_NAME_add_entry_by_txt(
      name, "CN", MBSTRING_ASC,
      reinterpret_cast<const unsigned char *>(cn.c_str()), -1, -1, 0);

  // Set issuer name to the name obj (self signed)
  X509_set_issuer_name(cert.get(), name);

  // Add extensions for TLS 1.3 auth
  add_extension(cert, NID_basic_constraints, "critical,CA:FALSE");
  add_extension(cert, NID_key_usage, "critical,digitalSignature");
  add_extension(cert, NID_ext_key_usage, "serverAuth,clientAuth");

  // Add Subject Alternate Name if present
  if (!san.empty())
    add_extension(cert, NID_subject_alt_name, san.c_str());

  // Assuming ed25519, sign with built-in digest (by passing nullptr as message
  // digest)
  if (!X509_sign(cert.get(), key.get(), nullptr))
    throw std::runtime_error("X509_sign failed");

  // done!

  return cert;
};

} // namespace ripple::util::cert