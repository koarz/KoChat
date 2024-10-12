#pragma once

#include "chat/chat_participant.hpp"
#include "duckdb.hpp"
#include "log/klog.hpp"
#include <deque>
#include <set>

class chat_room {
public:
  chat_room(koarz::LOG &log, duckdb::Connection &db) : log_(log), db_(db) {}
  void join(chat_participant_ptr participant) {
    log_(INFO, "new connect\n");
    participants_.insert(participant);
    for (auto msg : recent_msgs_)
      participant->deliver(std::format("{}: {}\n", msg.first, msg.second));
  }

  void leave(chat_participant_ptr participant) {
    log_(INFO,
         std::format("User {} leave room\n", participant->get_user_name()));
    participants_.erase(participant);
  }

  void deliver(const std::string &user_name, const std::string &msg) {
    recent_msgs_.push_back(std::make_pair(user_name, msg));
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    for (auto participant : participants_) {
      if (participant->get_user_name() != user_name)
        participant->deliver(std::format("{}: {}\n", user_name, msg));
    }
  }

  bool sign_in(std::string &user_name, std::string passwd) {
    user_name.pop_back();
    log_(INFO, "User Want Sign In\n");
    auto result = db_.Query(
        std::format("select * from user_info where user_name='{}'", user_name));
    if (result->RowCount() == 0) {
      return false;
    }
    if (result->GetValue(1, 0).ToString() != passwd) {
      return false;
    }
    log_(INFO, std::format("User {} get in room\n", user_name));
    return true;
  }

  bool create_new_account(std::string &user_name, std::string passwd) {
    user_name.pop_back();
    log_(INFO, "Create New Account\n");
    auto result = db_.Query(
        std::format("select * from user_info where user_name='{}'", user_name));
    if (result->RowCount() != 0) {
      return false;
    }
    db_.Query(std::format("insert into user_info values( '{}', '{}')",
                          user_name, passwd));
    log_(INFO, std::format("Create a new account: {}", user_name));
    log_(INFO, std::format("User {} get in room\n", user_name));
    return true;
  }

private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 };
  std::deque<std::pair<std::string, std::string>> recent_msgs_;
  koarz::LOG &log_;
  duckdb::Connection &db_;
};