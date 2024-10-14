#pragma once

#include "asio.hpp"
#include "asio/buffer.hpp"
#include "chat/char_room.hpp"
#include "chat/chat_participant.hpp"
#include <algorithm>
#include <string>

using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::redirect_error;
using asio::use_awaitable;
using asio::ip::tcp;

class chat_session : public chat_participant, public std::enable_shared_from_this<chat_session> {
public:
  chat_session(tcp::socket socket, chat_room &room)
      : socket_(std::move(socket)), timer_(socket_.get_executor()), room_(room) {
    timer_.expires_at(std::chrono::steady_clock::time_point::max());
  }

  void start() {
    std::string buffer;
    auto buffer_reader = asio::dynamic_buffer(buffer);

    while (true) {
      asio::read_until(socket_, buffer_reader, "\n");
      user_name = buffer.substr(1, buffer.find(' ') - 1);
      std::string passwd = buffer.substr(buffer.find(' ') + 1);
      if (buffer[0] == 'I') {
        // Sign in
        if (!room_.sign_in(user_name, passwd)) {
          buffer_reader.consume(buffer.size());
          continue;
        }
      } else {
        // Sign up
        if (!room_.sign_up(user_name, passwd)) {
          buffer_reader.consume(buffer.size());
          continue;
        }
      }
      asio::write(socket_, asio::buffer("success\n"));
      buffer_reader.consume(buffer.size());
      break;
    }

    room_.join(shared_from_this());

    co_spawn(
        socket_.get_executor(), [self = shared_from_this()] { return self->reader(); }, detached);

    co_spawn(
        socket_.get_executor(), [self = shared_from_this()] { return self->writer(); }, detached);
  }

  void deliver(const std::string &msg) {
    write_msgs_.push_back(msg);
    timer_.cancel_one();
  }

  std::string get_user_name() { return user_name; }

private:
  awaitable<void> reader() {
    try {
      for (std::string read_msg;;) {
        std::size_t n = co_await asio::async_read_until(
            socket_, asio::dynamic_buffer(read_msg, 1024), "\n", use_awaitable);

        room_.deliver(user_name, read_msg.substr(0, n - 1));
        read_msg.erase(0, n);
      }
    } catch (std::exception &) { stop(); }
  }

  awaitable<void> writer() {
    try {
      while (socket_.is_open()) {
        if (write_msgs_.empty()) {
          asio::error_code ec;
          co_await timer_.async_wait(redirect_error(use_awaitable, ec));
        } else {
          co_await asio::async_write(socket_, asio::buffer(write_msgs_.front()), use_awaitable);
          write_msgs_.pop_front();
        }
      }
    } catch (std::exception &) { stop(); }
  }

  void stop() {
    room_.leave(shared_from_this());
    socket_.close();
    timer_.cancel();
  }

  tcp::socket socket_;
  asio::steady_timer timer_;
  chat_room &room_;
  std::string user_name;
  std::deque<std::string> write_msgs_;
};
