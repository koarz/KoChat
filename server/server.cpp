#include "duckdb/duckdb.hpp"
#include <boost/asio.hpp>

int main() {
  boost::asio::io_context ctx;
  boost::asio::ip::tcp::socket socket{ctx};
  boost::asio::ip::tcp::endpoint endpoint{
      boost::asio::ip::address::from_string("127.0.0.1"), 1214};
  socket.connect(endpoint);
  boost::system::error_code ec;
  ec = socket.connect(endpoint, ec);

  return 0;
}