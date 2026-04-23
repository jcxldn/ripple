#ifndef CERT_CERT_HPP_
#define CERT_CERT_HPP_

#include "openssl/crypto.h"
#include "ripple/util/cert/common.hpp"

namespace ripple::util::cert {

cert_ptr create_self_signed_cert(key_ptr &key, const std::string &cn,
                                 const std::string &san, int validity_days);

} // namespace ripple::util::cert

#endif /* CERT_CERT_HPP_ */