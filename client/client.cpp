#include <iostream>
#include <memory>
#include <regex>
#include <string>

#include "asio.hpp"
#include "config/config.hpp"
#include "ftxui/component/component.hpp"          // for Input, Renderer, Vertical
#include "ftxui/component/component_base.hpp"     // for ComponentBase
#include "ftxui/component/screen_interactive.hpp" // for Component, ScreenInteractive
#include "ftxui/dom/elements.hpp" // for text, hbox, separator, Element, operator|, vbox, border
#include "ftxui/util/ref.hpp"     // for Ref

using asio::ip::tcp;
using namespace ftxui;

class chat_client {
public:
  chat_client(asio::io_context &io_context, const std::string host, const std::string port)
      : socket_(io_context), screen_(ScreenInteractive::TerminalOutput()) {
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(host, port);
    asio::async_connect(socket_, endpoints, [this](std::error_code ec, tcp::endpoint) {
      if (!ec) {
        home();
      } else {
        std::cerr << ec.message();
        socket_.close();
      }
    });
  }

public:
  void home() {
    std::string user_name;
    std::string password;
    std::string mess = "Input Your User name and Password";
    std::regex pattern("^[a-zA-Z0-9._-]+$");

    auto input_user_name = Input(&user_name, "UserName");
    auto input_password = Input(&password, "Password");

    // Use U and I to distinction operation
    auto button_sign_in = Button("Sign In",
                                 [&]() {
                                   if (!std::regex_match(user_name, pattern) ||
                                       !std::regex_match(password, pattern)) {
                                     mess = "Only allowed 0-9, a-z, A-Z and ._- charactor";
                                     return;
                                   }
                                   auto message = std::format("I{} {}\n", user_name, password);
                                   asio::write(socket_, asio::buffer(message));
                                   mess = "Waitting server return...";
                                   asio::read_until(socket_, asio::dynamic_buffer(read_msg_), "\n");
                                   if (read_msg_.substr(0, read_msg_.find('\n')) == "success") {
                                     read_msg_.clear();
                                     screen_.ExitLoopClosure();
                                     chat();
                                     return;
                                   } else {
                                     read_msg_.clear();
                                     mess = "Your username or password is incorrent";
                                   }
                                 }) |
                          flex;
    auto button_sign_up = Button("Sign Up",
                                 [&]() {
                                   if (!std::regex_match(user_name, pattern) ||
                                       !std::regex_match(password, pattern)) {
                                     mess = "Only allowed 0-9, a-z, A-Z and ._- charactor";
                                     return;
                                   }
                                   auto message = std::format("U{} {}\n", user_name, password);
                                   asio::async_write(socket_, asio::buffer(message),
                                                     [](std::error_code, std::size_t) {});
                                   mess = "Waitting server return...";
                                   asio::read_until(socket_, asio::dynamic_buffer(read_msg_), "\n");
                                   if (read_msg_.substr(0, read_msg_.find('\n')) == "success") {
                                     read_msg_.clear();
                                     screen_.ExitLoopClosure();
                                     chat();
                                     return;
                                   } else {
                                     read_msg_.clear();
                                     mess = "The Username already exists\n";
                                   }
                                 }) |
                          flex;

    auto component =
        Container::Vertical({input_user_name, input_password, button_sign_in, button_sign_up});

    auto home_renderer = Renderer(component, [&] {
      return vbox({hbox(text(mess)), hbox(text(" Username   : "), input_user_name->Render()),
                   hbox(text(" Password   : "), input_password->Render()), separator(),
                   hbox(button_sign_in->Render()), hbox(button_sign_up->Render())}) |
             border;
    });

    screen_.Loop(home_renderer);
  }

  void chat() {

    std::string input_value;

    int message_selected = 0;
    Component messages = Menu(&data_, &message_selected);

    auto input = Input(&input_value, "Enter text");

    auto button_send = Button("Send", [&] {
      if (input_value.empty()) {
        return;
      }
      if (*input_value.cbegin() != '\n') {
        input_value += '\n';
      }
      asio::write(socket_, asio::buffer(input_value));
      input_value.clear();
    });

    auto input_and_button = Container::Horizontal({
        input,
        button_send,
    });

    auto component = Container::Vertical({messages, input_and_button});

    auto renderer = Renderer(component, [&] {
      auto mess_win = window(text("message"), messages->Render() | vscroll_indicator | frame |
                                                  size(HEIGHT, EQUAL, 10) | border);
      return vbox({
          mess_win,
          separator(),
          input_and_button->Render() | border,
      });
    });

    std::thread([this] { read(); }).detach();

    screen_.Loop(renderer);
  }

  void read() {
    for (;;) {
      auto n = asio::read_until(socket_, asio::dynamic_buffer(read_msg_), "\n");
      data_.push_back(read_msg_.substr(0, n));
      read_msg_.erase(0, n);
    }
  }

  ScreenInteractive screen_;
  tcp::socket socket_;
  std::string read_msg_;
  std::vector<std::string> data_;
};

int main(int argc, char *argv[]) {
  try {
    asio::io_context io_context;
    chat_client client(io_context, host, std::to_string(port));
    io_context.run();
  } catch (std::exception &e) { std::cerr << "Exception: " << e.what() << std::endl; }
  return 0;
}
