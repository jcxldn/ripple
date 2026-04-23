#ifndef LOGGER_LOGGER_HPP_
#define LOGGER_LOGGER_HPP_

// #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <mutex>

namespace ripple::logger {

using logger = spdlog::logger;

class LoggerProvider {
private:
  static std::once_flag flag;
  static void bootstrap();

  static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> console;

  static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>>
      loggers;

public:
  static void ensure();

  static std::shared_ptr<spdlog::logger> get_logger(std::string name);

  static void set_level(spdlog::level::level_enum level);
};

} // namespace ripple::logger

#endif /* LOGGER_LOGGER_HPP_ */