#pragma once

#include <memory>

#include "base/memory/weak_ptr.h"

#include "engine/backends/steam/steam_auth_backend.h"
#include "models/auth.h"
#include "utils/subprocess.h"

namespace engine::backend {

class SteamSubprocessAuthBackend : public SteamAuthBackend {
 public:
  static void RunInSubprocessAndDie(
      // NOLINTBEGIN(modernize-avoid-c-arrays)
      int argc,
      char* argv[]
      // NOLINTEND(modernize-avoid-c-arrays)
  );

  // `steam_auth_backend_command` has to point to executable that will take
  // additional `--pipe_handle=<...>` argument and write auth data to that pipe
  SteamSubprocessAuthBackend(int steam_appid,
                             AuthType auth_type,
                             std::string steam_auth_backend_command);
  ~SteamSubprocessAuthBackend() override;

  void Authenticate(
      base::OnceCallback<void(model::AuthResult)> on_done_callback) override;

 private:
  void ResetAuthSubprocess();

  std::string steam_auth_backend_command_;
  std::unique_ptr<util::Subprocess> auth_subprocess_;

  base::WeakPtr<SteamSubprocessAuthBackend> weak_this_;
  base::WeakPtrFactory<SteamSubprocessAuthBackend> weak_factory_;
};

}  // namespace engine::backend
