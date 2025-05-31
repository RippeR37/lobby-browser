#pragma once

#include <string>

namespace model {

struct AuthResult {
  bool success;
  std::string error;

  std::string user_name;
  std::string user_id;
  std::string auth_token;
};

}  // namespace model
