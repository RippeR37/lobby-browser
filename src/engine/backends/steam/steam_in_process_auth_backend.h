#pragma once

#include "base/threading/thread.h"

#include "engine/backends/steam/steam_auth_backend.h"
#include "models/auth.h"

namespace engine::backend {

class SteamInProcessAuthBackend : public SteamAuthBackend {
 public:
  SteamInProcessAuthBackend(int appid, AuthType auth_type);
  ~SteamInProcessAuthBackend() override;

  void Authenticate(
      base::OnceCallback<void(model::AuthResult)> on_done_callback) override;

 private:
  base::Thread steam_thread_;
};

}  // namespace engine::backend
