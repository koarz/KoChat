#pragma once
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iostream>
#include <mutex>
#include <thread>

namespace koarz {
class LOG {
  std::fstream fs;
  uint64_t flags{};
  std::mutex mut;
  bool close{};

  static constexpr uint64_t TIME = 1 << 0;      // Output time information
  static constexpr uint64_t REDIRFILE = 1 << 1; // Redirection of output files
  static constexpr uint64_t THREADID = 1 << 2;  // Output thread id

public:
  enum class LEVEL { OFF, FATAL, ERROR, WARN, INFO, DEBUG };
  LOG() = default;
  ~LOG() {
    if (fs.is_open())
      fs.close();
  };

  void show_time() { flags |= TIME; }
  void close_show_time() { flags &= ~TIME; }
  void redirect_to_file(std::filesystem::path path) {
    flags |= REDIRFILE;
    std::unique_lock<std::mutex> lock(mut);
    if (fs.is_open())
      fs.close();
    fs.open(path, std::ios_base::out | std::ios_base::ate | std::ios_base::app);
  }
  void close_file() {
    flags &= ~REDIRFILE;
    std::unique_lock<std::mutex> lock(mut);
    if (fs.is_open())
      fs.close();
  }
  void show_thread_id() { flags |= THREADID; }
  void close_show_thread_id() { flags &= ~THREADID; }
  void reopen_log() { close = false; }

  template <typename T> void operator()(LOG::LEVEL level, T &&sv) {
    if (close) {
      return;
    }
    std::stringstream info;
    if (flags & THREADID) {
      info << "Thread : " << std::this_thread::get_id() << ' ';
    }
    if (flags & TIME) {
      const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      struct tm cutTm = {0};
      info << std::put_time(localtime_r(&now, &cutTm), "%F %T ");
    }
    std::string data;
    switch (level) {
    case LEVEL::OFF:
      data = std::format("\033[36m{} [OFF] {}", info.str(), sv);
      close = true;
      break;
    case LEVEL::FATAL: data = std::format("\033[30m{} [FATAL] {}", info.str(), sv); break;
    case LEVEL::ERROR: data = std::format("\033[31m{} [ERROR] {}", info.str(), sv); break;
    case LEVEL::WARN: data = std::format("\033[33m{} [WARN] {}", info.str(), sv); break;
    case LEVEL::INFO: data = std::format("\033[37m{} [INFO] {}", info.str(), sv); break;
    case LEVEL::DEBUG: data = std::format("\033[35m{} [DEBUG] {}", info.str(), sv); break;
    }
    std::unique_lock<std::mutex> lock(mut);
    if (flags & REDIRFILE) {
      fs << data;
    } else {
      std::cerr << data;
    }
  }
};
} // namespace koarz

#define OFF koarz::LOG::LEVEL::OFF
#define FATAL koarz::LOG::LEVEL::FATAL
#define ERROR koarz::LOG::LEVEL::ERROR
#define WARN koarz::LOG::LEVEL::WARN
#define DEBUG koarz::LOG::LEVEL::DEBUG
#define INFO koarz::LOG::LEVEL::INFO
#define TRACE koarz::LOG::LEVEL::TRACE