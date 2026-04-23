#ifndef CERT_IDENTITY_HPP_
#define CERT_IDENTITY_HPP_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace ripple::util::cert {

class Identity {
private:
  std::string cert_pem; // PEM-encoded X.509
  std::string key_pem;  // PEM-encoded PKCS#8 private key (unencrypted)
  std::vector<uint8_t> spki_sha256; // 32 bytes, for pubkey pinning

  // copies of original passed to constructor
  std::string cn;
  std::string san;

public:
  Identity(const std::string &cn = "ripple-default",
           const std::string &san = "", int validity_days = 365);

  std::string spki_b64();

  inline std::string get_cn() { return cn; };
  inline std::string get_san() { return san; };
};

using id_ptr = std::shared_ptr<Identity>;

} // namespace ripple::util::cert

#endif /* CERT_IDENTITY_HPP_ */