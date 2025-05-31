#pragma once

#include "engine/backends/auth_backend.h"

namespace engine::backend {

class SteamAuthBackend : public AuthBackend {
 public:
  enum class AuthType {
    kEncryptedAppTicket,
    kAuthSessionTicket,
  };

  SteamAuthBackend(int appid, AuthType auth_type);
  ~SteamAuthBackend() override;

 protected:
  int GetAppId() const;
  AuthType GetAuthType() const;

 private:
  int appid_;
  AuthType auth_type_;
};

}  // namespace engine::backend
