#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/net/resource_response.h"

#include "engine/backends/auth_token.h"
#include "engine/backends/lobby_backend.h"
#include "engine/backends/nakama/nakama_data.h"
#include "engine/backends/result.h"
#include "engine/backends/steam/steam_auth_backend.h"

namespace engine::backend {

class NakamaLobbyBackend : public LobbyBackend {
 public:
  NakamaLobbyBackend(std::string server_url,
                     std::string auth_init_token,
                     std::unique_ptr<SteamAuthBackend> steam_auth_backend);
  ~NakamaLobbyBackend() override;

  void SearchLobbies(
      nakama::LobbySearchRequest request,
      base::OnceCallback<void(Result, nakama::LobbySearchResponse)>
          on_done_callback);

 private:
  void StartAuthenticateViaSteam();
  void AuthenticateWithSteam(model::AuthResult auth_result);
  void OnNakamaAuthResponse(base::net::ResourceResponse response);
  void ProcessPendingRequests(std::optional<Result> error_result);

  void DoSearchLobbies(
      nakama::LobbySearchRequest request,
      base::OnceCallback<void(Result, nakama::LobbySearchResponse)>
          on_done_callback,
      std::optional<Result> error_result);
  void OnSearchLobbiesResponse(
      base::OnceCallback<void(Result, nakama::LobbySearchResponse)>
          on_done_callback,
      base::net::ResourceResponse response);

  std::string server_url_;
  std::string auth_init_token_;
  std::unique_ptr<SteamAuthBackend> steam_auth_backend_;
  AuthToken<std::string> auth_token_;

  bool pending_auth_response_;
  std::vector<base::OnceCallback<void(std::optional<Result>)>>
      requests_pending_auth_;

  base::WeakPtr<NakamaLobbyBackend> weak_this_;
  base::WeakPtrFactory<NakamaLobbyBackend> weak_factory_;
};

}  // namespace engine::backend
