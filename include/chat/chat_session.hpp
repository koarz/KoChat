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

class chat_session : public chat_participant,
                     public std::enable_shared_from_this<chat_session> {
public:
  chat_session(tcp::socket socket, chat_room &room)
      : socket_(std::move(socket)), timer_(socket_.get_executor()),
        room_(room) {
    timer_.expires_at(std::chrono::steady_clock::time_point::max());
  }

  void start() {
    std::string buffer;
    auto buffer_reader = asio::dynamic_buffer(buffer);
    auto name = asio::dynamic_buffer(user_name);

    asio::write(socket_,
                asio::buffer("Input 1 for Sign In 2 for Create New Account\n"));
    while (true) {
      asio::read_until(socket_, buffer_reader, "\n");
      if (std::all_of(buffer.begin(), buffer.end() - 1,
                      [&](auto &v) { return v >= '0' && v <= '9'; })) {
        break;
      }
      asio::write(socket_, asio::buffer("Please input 1 or 2\n"));
      buffer_reader.consume(buffer.size());
    }
    switch (std::stoi(buffer)) {
    // Sign In
    case 1:
      asio::write(
          socket_,
          asio::buffer("[Sign In]Input your account name and password\n"));
      while (true) {
        buffer_reader.consume(buffer.size());
        name.consume(user_name.size());
        asio::read_until(socket_, name, '\n');
        asio::read_until(socket_, buffer_reader, '\n');
        if (room_.sign_in(user_name, buffer)) {
          break;
        }
        asio::write(socket_, asio::buffer("Your account name "
                                          "or password is incorrect\n"));
      }

      break;
    // Create New Account And Sign In
    case 2:
      asio::write(
          socket_,
          asio::buffer(
              "[Create Account]Input your account name and password\n"));
      while (true) {
        buffer_reader.consume(buffer.size());
        name.consume(user_name.size());
        asio::read_until(socket_, name, '\n');
        asio::read_until(socket_, buffer_reader, '\n');
        if (room_.create_new_account(user_name, buffer)) {
          break;
        }
        asio::write(socket_, asio::buffer("The name already exists\n"));
      }
      break;
    default:
      start();
      return;
    }

    room_.join(shared_from_this());

    co_spawn(
        socket_.get_executor(),
        [self = shared_from_this()] { return self->reader(); }, detached);

    co_spawn(
        socket_.get_executor(),
        [self = shared_from_this()] { return self->writer(); }, detached);
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
    } catch (std::exception &) {
      stop();
    }
  }

  awaitable<void> writer() {
    try {
      while (socket_.is_open()) {
        if (write_msgs_.empty()) {
          asio::error_code ec;
          co_await timer_.async_wait(redirect_error(use_awaitable, ec));
        } else {
          co_await asio::async_write(socket_, asio::buffer(write_msgs_.front()),
                                     use_awaitable);
          write_msgs_.pop_front();
        }
      }
    } catch (std::exception &) {
      stop();
    }
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
