#include "utils/subprocess.h"

#include <array>

#include "base/logging.h"

#include "windows.h"

namespace util {

namespace {
std::string BuildEnvVarsBlock(const std::vector<std::string>& env_vars) {
  std::string block;

  for (const auto& env_var : env_vars) {
    block.insert(block.end(), env_var.begin(), env_var.end());
    block.push_back('\0');
  }
  block.push_back('\0');

  return block;
}

Subprocess::Result SpawnWinSubprocessAndWaitForResults(
    std::string command,
    std::vector<std::string> arguments,
    std::vector<std::string> env_vars,
    std::string pipe_arg_name) {
  // Allow handle inheritance
  SECURITY_ATTRIBUTES sa = {sizeof(sa), nullptr, TRUE};
  HANDLE read_pipe = nullptr, write_pipe = nullptr;

  if (!CreatePipe(&read_pipe, &write_pipe, &sa, 0)) {
    LOG(ERROR) << "Failed to create pipe: " << GetLastError();
    return Subprocess::Result{1, ""};
  }

  const auto pipe_handle_str =
      std::to_string(reinterpret_cast<uintptr_t>(write_pipe));

  arguments.emplace_back(pipe_arg_name + "=" + pipe_handle_str);
  for (const auto& argument : arguments) {
    command += " " + argument;
  }

  // Prevent parent from writing
  SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOA si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi = {};
  auto env_block = BuildEnvVarsBlock(env_vars);

  if (!CreateProcessA(nullptr, command.data(), nullptr, nullptr, TRUE,
                      CREATE_NO_WINDOW, env_block.data(), nullptr, &si, &pi)) {
    LOG(ERROR) << "Failed to create subprocess: " << GetLastError();
    return Subprocess::Result{2, ""};
  }

  CloseHandle(write_pipe);  // Parent doesn't write

  // Read message
  std::string received_data;
  std::array<char, 256> buffer{};
  DWORD bytes_read = 0;
  while (
      ReadFile(read_pipe, buffer.data(), buffer.size(), &bytes_read, nullptr)) {
    received_data.append(buffer.data(), bytes_read);
  }

  CloseHandle(read_pipe);
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return Subprocess::Result{0, std::move(received_data)};
}
}  // namespace

// static
void Subprocess::WriteToSubprocessPipe(std::string pipe, std::string message) {
  auto* write_pipe = reinterpret_cast<HANDLE>(std::stoull(pipe));

  DWORD written = 0;
  WriteFile(write_pipe, message.c_str(), message.size(), &written, nullptr);
  CloseHandle(write_pipe);
}

Subprocess::Subprocess() = default;

Subprocess::~Subprocess() = default;

void Subprocess::Execute(std::string command,
                         std::vector<std::string> arguments,
                         std::vector<std::string> env_vars,
                         std::string pipe_arg_name,
                         base::OnceCallback<void(Result)> on_done_callback) {
  monitor_thread_.Stop();
  monitor_thread_.Start();
  monitor_thread_.TaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&SpawnWinSubprocessAndWaitForResults, std::move(command),
                     std::move(arguments), std::move(env_vars),
                     std::move(pipe_arg_name)),
      std::move(on_done_callback));
}

}  // namespace util
