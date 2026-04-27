#ifndef SERIALIZATION_CEREAL_HPP_
#define SERIALIZATION_CEREAL_HPP_

#include <boost/asio/buffer.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/memory.hpp>
#include <istream>
#include <memory>
#include <sstream>

#include "ripple/logger/logger.hpp"
namespace ripple::serialization {

struct SerializedData {
  boost::asio::mutable_buffer buf;
  std::string data;
};

class membuf : public std::streambuf {
public:
  membuf(const char *data, std::size_t size) {
    char *p = const_cast<char *>(data);
    setg(p, p, p + size);
  }
};

template <typename T>
std::shared_ptr<T> deserialize(boost::asio::const_buffer buf) {
  auto *data = static_cast<const char *>(buf.data());
  auto size = buf.size();

  membuf sbuf(data, size);
  std::istream is(&sbuf);

  ::cereal::PortableBinaryInputArchive ar(is);

  std::shared_ptr<T> value;
  ar(value);
  return value;
};

template <typename T>
std::shared_ptr<SerializedData> serialize(std::shared_ptr<T> value) {
  std::ostringstream os;
  cereal::PortableBinaryOutputArchive ar(os);

  ar(value);

  auto data = std::make_shared<SerializedData>();
  data->data = os.str();
  data->buf = boost::asio::buffer(data->data);

  logger::LoggerProvider::get_logger("ripple::serialization::cereal")
      ->trace("Serialized {} bytes", data->buf.size());

  return data;
};

}; // namespace ripple::serialization
#endif /* SERIALIZATION_CEREAL_HPP_ */