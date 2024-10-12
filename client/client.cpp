#include "asio.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <thread>

using asio::ip::tcp;

class chat_client {
public:
  chat_client(asio::io_context &io_context, const std::string &host,
              const std::string &port)
      : socket_(io_context) {
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(host, port);
    asio::async_connect(socket_, endpoints,
                        [this](std::error_code ec, tcp::endpoint) {
                          if (!ec) {
                            read();
                            write();
                          } else {
                            std::cerr << ec.message();
                            socket_.close();
                          }
                        });
  }

private:
  void read() {
    asio::async_read_until(socket_, asio::dynamic_buffer(read_msg_), "\n",
                           [this](std::error_code ec, std::size_t length) {
                             if (!ec) {
                               std::cout << read_msg_.substr(0, length);
                               read_msg_.erase(0, length);
                               read();
                             } else {
                               std::cerr << ec.message();
                               socket_.close();
                             }
                           });
  }

  void write() {
    std::thread([this]() {
      for (;;) {
        std::string message;
        std::getline(std::cin, message);
        message += "\n";
        asio::async_write(socket_, asio::buffer(message),
                          [](std::error_code, std::size_t) {});
      }
    }).detach();
  }

  tcp::socket socket_;
  std::string read_msg_;
};

int main(int argc, char *argv[]) {
  try {

    asio::io_context io_context;
    chat_client client(io_context, "127.0.0.1", "1111");
    io_context.run();
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
  return 0;
}
