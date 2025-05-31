#pragma once

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/threading/thread.h"

namespace util {

class Subprocess {
 public:
  struct Result {
    int exit_code;
    std::string pipe_data;
  };

  static void WriteToSubprocessPipe(std::string pipe, std::string message);

  Subprocess();
  ~Subprocess();

  void Execute(std::string command,
               std::vector<std::string> arguments,
               std::vector<std::string> env_vars,
               std::string pipe_arg_name,
               base::OnceCallback<void(Result)> on_done_callback);

 private:
  base::Thread monitor_thread_;
};

}  // namespace util
