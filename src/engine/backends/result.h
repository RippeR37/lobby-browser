#pragma once

#include <string>

namespace engine::backend {

struct Result {
  enum class Status {
    kOk,
    kAuthFailed,
    kFailed,
  };

  Status status;
  std::string error;
};

}  // namespace engine::backend
