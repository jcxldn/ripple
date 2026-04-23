#include "ripple/logger/logger.hpp"

namespace ripple::logger {

std::once_flag LoggerProvider::flag;

std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> LoggerProvider::console =
    nullptr;

std::unordered_map<std::string, std::shared_ptr<spdlog::logger>>
    LoggerProvider::loggers;

void LoggerProvider::ensure() { std::call_once(flag, bootstrap); };

void LoggerProvider::bootstrap() {
  console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
};

void LoggerProvider::set_level(spdlog::level::level_enum log_level) {
  ensure(); // call bootstrap if not already called
  console->set_level(log_level);
  for (auto &logger : loggers) {
    logger.second->set_level(log_level);
  }
};

std::shared_ptr<spdlog::logger> LoggerProvider::get_logger(std::string name) {
  ensure(); // call bootstrap if not already called

  std::shared_ptr<spdlog::logger> new_logger =
      std::make_shared<spdlog::logger>(name, console);
  new_logger->set_level(console->level());

  auto result = loggers.emplace(name, std::move(new_logger));

  // bool was_created = result.second;

  return result.first->second;
}

} // namespace ripple::logger
