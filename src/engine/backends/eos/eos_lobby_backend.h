#pragma once

#include "engine/backends/lobby_backend.h"

#include <string>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/net/resource_response.h"

#include "engine/backends/auth_token.h"
#include "engine/backends/eos/eos_data.h"
#include "engine/backends/result.h"
#include "engine/backends/steam/steam_auth_backend.h"
#include "models/auth.h"

namespace engine::backend {

class EosLobbyBackend : public LobbyBackend {
 public:
  EosLobbyBackend(std::string auth_init_token,
                  std::string deployment_id,
                  std::string nonce,
                  std::unique_ptr<SteamAuthBackend> steam_auth_backend);
  ~EosLobbyBackend() override;

  void SearchLobbies(
      eos::SearchLobbiesRequest request,
      base::OnceCallback<void(Result, eos::SearchLobbiesResponse)>
          on_done_callback);

 private:
  void StartAuthenticateViaSteam();
  void AuthenticateWithSteam(model::AuthResult auth_result);
  void OnEosAuthResponse(base::net::ResourceResponse response);
  void ProcessPendingRequests(std::optional<Result> error_result);

  void DoSearchLobbies(
      std::string json_request,
      base::OnceCallback<void(Result, eos::SearchLobbiesResponse)>
          on_done_callback,
      std::optional<Result> error_result);
  void OnSearchLobbiesResponse(
      base::OnceCallback<void(Result, eos::SearchLobbiesResponse)>
          on_done_callback,
      base::net::ResourceResponse response);

  std::string auth_init_token_;
  std::string deployment_id_;
  std::string nonce_;
  std::unique_ptr<SteamAuthBackend> steam_auth_backend_;
  AuthToken<std::string> auth_token_;

  bool pending_auth_response_;
  std::vector<base::OnceCallback<void(std::optional<Result>)>>
      requests_pending_auth_;

  base::WeakPtr<EosLobbyBackend> weak_this_;
  base::WeakPtrFactory<EosLobbyBackend> weak_factory_;
};

}  // namespace engine::backend
