#include "chat/char_room.hpp"
#include "chat/chat_session.hpp"
#include "config/config.hpp"
#include "duckdb.hpp"
#include "log/klog.hpp"

#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read_until.hpp>
#include <asio/redirect_error.hpp>
#include <asio/signal_set.hpp>
#include <asio/steady_timer.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>
#include <cstdlib>
#include <memory>

using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using asio::ip::tcp;

koarz::LOG log;

asio::thread_pool thread_pool(running_thread);

awaitable<void> listener(tcp::acceptor acceptor, duckdb::Connection &con) {
  chat_room room{con, log};

  for (;;) {
    auto chat_session_ =
        std::make_shared<chat_session>(co_await acceptor.async_accept(use_awaitable), room);
    asio::post(thread_pool, [chat_session_]() { chat_session_->start(); });
  }
}

int main(int argc, char *argv[]) {
  log.show_time();
  duckdb::DuckDB db("~/KoChat/data/info.db");
  duckdb::Connection con(db);
  // create new table for store user infomation
  con.Query("create table if not exists user_info(user_name string primary key, passwd string)");
  try {
    asio::io_context io_context(1);

    co_spawn(io_context, listener(tcp::acceptor(io_context, {tcp::v4(), port}), con), detached);

    asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) { io_context.stop(); });

    io_context.run();
  } catch (std::exception &e) { log(FATAL, e.what()); }
  con.Query(".quit");
  return 0;
}