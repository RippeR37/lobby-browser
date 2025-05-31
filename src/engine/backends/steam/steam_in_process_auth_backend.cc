#include "engine/backends/steam/steam_in_process_auth_backend.h"

#include "base/timer/elapsed_timer.h"
#include "steam_api.h"

#include "engine/backends/steam/steam_manager.h"

namespace engine::backend {

namespace {

void RunSteamEventLoop(steam::SteamManager* steam_manager) {
  if (steam_manager && steam_manager->IsInitialized()) {
    SteamAPI_RunCallbacks();
  }
}

model::AuthResult GetSteamAuthTicket(SteamAuthBackend::AuthType auth_type) {
  steam::SteamManager steam_manager{true};
  if (!steam_manager.IsInitialized()) {
    return model::AuthResult{
        false,
        "Failed to authenticate with Steam: failed to initialize Steam API", "",
        "", ""};
  }
  if (!steam_manager.IsLoggedIn()) {
    return model::AuthResult{
        false,
        "Failed to authenticate with Steam: cannot authenticate with Steam, "
        "user is not logged in",
        "", "", ""};
  }

  if (auth_type == SteamAuthBackend::AuthType::kAuthSessionTicket) {
    auto auth_session_ticket = steam_manager.GetAuthSessionTicket();

    // It seems that Steam needs time to process this before the ticket is
    // ready to be actually used for authentication, and there doesn't seem to
    // be API to wait until then, so let's run Steam's loop here a couple of
    // times and give it time to fully process it before we return it.
    const auto kMaxRuns = 5;
    for (size_t idx = 0; idx < kMaxRuns; ++idx) {
      RunSteamEventLoop(&steam_manager);
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return model::AuthResult{true, "", steam_manager.GetUserName(),
                             std::to_string(steam_manager.GetUserId()),
                             std::move(auth_session_ticket)};
  }

  bool done = false;
  steam::SteamManager::Result result;

  auto on_done_callback = base::BindOnce(
      [](steam::SteamManager::Result* outer_result_ptr, bool* done_flag,
         steam::SteamManager::Result result) {
        *outer_result_ptr = result;
        *done_flag = true;
      },
      &result, &done);

  base::ElapsedTimer timer;
  steam_manager.AsyncRequestEncryptedAppTicket(std::move(on_done_callback));

  do {
    RunSteamEventLoop(&steam_manager);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  } while (!done && timer.Elapsed() <= base::Seconds(10));

  if (!done) {
    return model::AuthResult{
        false,
        "Failed to authenticate with Steam: timed out on waiting "
        "for Steam auth ticket",
        "", "", ""};
  }

  return model::AuthResult{true, "", steam_manager.GetUserName(),
                           std::to_string(steam_manager.GetUserId()),
                           result.ticket};
}

}  // namespace

SteamInProcessAuthBackend::SteamInProcessAuthBackend(int appid,
                                                     AuthType auth_type)
    : SteamAuthBackend(appid, auth_type) {
  steam_thread_.Start();
}

SteamInProcessAuthBackend::~SteamInProcessAuthBackend() {
  steam_thread_.Stop();
}

void SteamInProcessAuthBackend::Authenticate(
    base::OnceCallback<void(model::AuthResult)> on_done_callback) {
  steam_thread_.TaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&GetSteamAuthTicket, GetAuthType()),
      std::move(on_done_callback));
}

}  // namespace engine::backend
