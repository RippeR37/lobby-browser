#include "engine/backends/steam/steam_subprocess_auth_backend.h"

#include <memory>

#include "base/callback.h"
#include "base/logging.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "nlohmann/json.hpp"

#include "engine/backends/steam/steam_in_process_auth_backend.h"
#include "models/serialize/auth.h"
#include "utils/arg_parse.h"

namespace engine::backend {

namespace {
model::AuthResult ConvertSubprocessResultToAuthResult(
    util::Subprocess::Result result) {
  nlohmann::json json_result =
      nlohmann::json::parse(std::move(result.pipe_data));
  model::AuthResult model_result = json_result;
  model_result.success = model_result.success && (result.exit_code == 0);
  return model_result;
}

void SubprocessTimeoutCleanUp(
    std::unique_ptr<SteamInProcessAuthBackend> backend,
    std::string pipe_handle) {
  backend.reset();
  model::AuthResult fail_result{
      false, "Failed to authenticate with Steam - timeout", {}, {}, {}};
  util::Subprocess::WriteToSubprocessPipe(pipe_handle,
                                          nlohmann::json(fail_result).dump());
  exit(1);
}

std::string ToString(SteamAuthBackend::AuthType auth_type) {
  switch (auth_type) {
    case SteamAuthBackend::AuthType::kEncryptedAppTicket:
      return "encrypted-app-ticket";
    case SteamAuthBackend::AuthType::kAuthSessionTicket:
      return "auth-session-ticket";
  }
}
}  // namespace

// static
void SteamSubprocessAuthBackend::RunInSubprocessAndDie(
    // NOLINTBEGIN(modernize-avoid-c-arrays)
    int argc,
    char* argv[]
    // NOLINTEND(modernize-avoid-c-arrays)
) {
  const auto pipe_handle = util::ParseArg(argc, argv, "--pipe-handle=");
  if (!pipe_handle) {
    LOG(ERROR) << "No pipe handle provided, cannot continue.";
    exit(1);
  }

  auto auth_type_arg = util::ParseArg(argc, argv, "--auth-type=");
  auto auth_type = SteamAuthBackend::AuthType::kEncryptedAppTicket;
  if (auth_type_arg == "auth-session-ticket") {
    auth_type = SteamAuthBackend::AuthType::kAuthSessionTicket;
  }

  // Will use AppId from environment variable
  auto auth_backend =
      std::make_unique<SteamInProcessAuthBackend>(-1, auth_type);

  auth_backend->Authenticate(base::BindOnce(
      [](std::string pipe, model::AuthResult result) {
        auto result_json = nlohmann::json(result).dump();
        util::Subprocess::WriteToSubprocessPipe(pipe, std::move(result_json));
        exit(0);
      },
      *pipe_handle));

  // Auto-release backend and exit after specified timeout
  const auto kTimeout = base::Seconds(15);
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&SubprocessTimeoutCleanUp, std::move(auth_backend),
                     *pipe_handle),
      kTimeout);
}

SteamSubprocessAuthBackend::SteamSubprocessAuthBackend(
    int steam_appid,
    AuthType auth_type,
    std::string steam_auth_backend_command)
    : SteamAuthBackend(steam_appid, auth_type),
      steam_auth_backend_command_(std::move(steam_auth_backend_command)),
      weak_factory_(this) {
  DCHECK(!steam_auth_backend_command_.empty());
  DCHECK_GT(GetAppId(), 0);
  weak_this_ = weak_factory_.GetWeakPtr();
}

SteamSubprocessAuthBackend::~SteamSubprocessAuthBackend() = default;

void SteamSubprocessAuthBackend::Authenticate(
    base::OnceCallback<void(model::AuthResult)> on_done_callback) {
  auth_subprocess_ = std::make_unique<util::Subprocess>();
  auth_subprocess_->Execute(
      steam_auth_backend_command_,
      {
          "--auth-type=" + ToString(GetAuthType()),
      },
      {
          "STEAMAPPID=" + std::to_string(GetAppId()),
      },
      "--pipe-handle",
      base::BindOnce(&ConvertSubprocessResultToAuthResult)
          .Then(std::move(on_done_callback))
          .Then(base::BindOnce(&SteamSubprocessAuthBackend::ResetAuthSubprocess,
                               weak_this_)));
}

void SteamSubprocessAuthBackend::ResetAuthSubprocess() {
  auth_subprocess_.reset();
}

}  // namespace engine::backend
