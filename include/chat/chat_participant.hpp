#pragma once

#include <memory>
#include <string>

class chat_participant {
public:
  virtual ~chat_participant() {}
  virtual void deliver(const std::string &msg) = 0;
  virtual std::string get_user_name() = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;