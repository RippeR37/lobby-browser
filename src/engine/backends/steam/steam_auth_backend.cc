#include "engine/backends/steam/steam_auth_backend.h"

namespace engine::backend {

SteamAuthBackend::SteamAuthBackend(int appid, AuthType auth_type)
    : appid_(appid), auth_type_(auth_type) {}

SteamAuthBackend::~SteamAuthBackend() = default;

int SteamAuthBackend::GetAppId() const {
  return appid_;
}

SteamAuthBackend::AuthType SteamAuthBackend::GetAuthType() const {
  return auth_type_;
}

}  // namespace engine::backend
