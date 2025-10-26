#include "engine/backends/pavlov/pavlov_lobby_backend.h"

#include <memory>
#include <string>

#include "base/net/resource_request.h"
#include "base/net/simple_url_loader.h"
#include "nlohmann/json.hpp"

#include "engine/backends/pavlov/pavlov_data.h"
#include "engine/backends/steam/steam_subprocess_auth_backend.h"

namespace engine::backend {

namespace {
const std::string kPavlovEosAuthInitToken =
    "Basic "
    "eHl6YTc4OTFSbmRPSUhwTWlacmtQNXFkRE5ZRHJQZ3Y6ZlE2djdDVFVEdFYxVVo4bjdFOWpYQn"
    "hMMEZHSXhXbEVlZkNYV2VYR2czZw==";
const std::string kPavlovEosDeploymentId = "e708e7885689412aa634bef12ec60023";
const std::string kPavlovEosNonce = "yA5lpRhWbECU8XQ8Xrb44w";
const int kPavlovSteamAppId = 555160;

const auto* kSearchUsersUrl =
    "https://prod-crossplay-pavlov-ms.vankrupt.net/friends/v1/search";
const auto kSearchUsersTimeout = base::Seconds(15);
}  // namespace

PavlovLobbyBackend::PavlovLobbyBackend(std::string steam_auth_backend_command)
    : EosLobbyBackend(
          kPavlovEosAuthInitToken,
          kPavlovEosDeploymentId,
          kPavlovEosNonce,
          std::make_unique<backend::SteamSubprocessAuthBackend>(
              kPavlovSteamAppId,
              backend::SteamAuthBackend::AuthType::kEncryptedAppTicket,
              std::move(steam_auth_backend_command))),
      pavlov_weak_factory_(this) {
  pavlov_weak_this_ = pavlov_weak_factory_.GetWeakPtr();
}

PavlovLobbyBackend::~PavlovLobbyBackend() = default;

void PavlovLobbyBackend::SearchUsers(
    pavlov::SearchUsersRequest request,
    base::OnceCallback<void(Result, pavlov::SearchUsersResponse)>
        on_done_callback) {
  nlohmann::json json_request;
  try {
    json_request = request;
  } catch (const std::exception& e) {
    LOG(ERROR) << __FUNCTION__
               << "() failed to serialize request: " << e.what();
    std::move(on_done_callback)
        .Run(Result{Result::Status::kFailed,
                    "Failed to serialize search request" +
                        std::string(e.what())},
             {});
    return;
  }

  if (!id_token_.IsValid()) {
    requests_pending_auth_.emplace_back(
        base::BindOnce(&PavlovLobbyBackend::DoSearchUsers, pavlov_weak_this_,
                       json_request.dump(), std::move(on_done_callback)));
    if (!pending_auth_response_) {
      StartAuthenticateViaSteam();
    }
    return;
  }

  DoSearchUsers(json_request.dump(), std::move(on_done_callback), {});
}

void PavlovLobbyBackend::DoSearchUsers(
    std::string json_request,
    base::OnceCallback<void(Result, pavlov::SearchUsersResponse)>
        on_done_callback,
    std::optional<Result> error_result) {
  if (!id_token_.IsValid()) {
    std::move(on_done_callback)
        .Run(error_result.value_or(
                 Result{Result::Status::kAuthFailed, "Failed to authenticate"}),
             pavlov::SearchUsersResponse{});
    return;
  }

  const auto kMaxResponseSize = 5 * 1024 * 1024;
  base::net::SimpleUrlLoader::DownloadLimited(
      base::net::ResourceRequest{kSearchUsersUrl}
          .WithHeaders({
              std::string("access-token: ") + *id_token_.GetToken(),
              std::string("Content-Type: text/json"),
              std::string("User-Agent: Pavlov VR Game Client"),
          })
          .WithPostData(std::move(json_request))
          .WithTimeout(kSearchUsersTimeout),
      kMaxResponseSize,
      base::BindOnce(&PavlovLobbyBackend::OnSearchUsersResponse,
                     pavlov_weak_this_, std::move(on_done_callback)));
}

void PavlovLobbyBackend::OnSearchUsersResponse(
    base::OnceCallback<void(Result, pavlov::SearchUsersResponse)>
        on_done_callback,
    base::net::ResourceResponse response) {
  if (response.result != base::net::Result::kOk) {
    std::move(on_done_callback)
        .Run(
            Result{Result::Status::kFailed,
                   "Failed to search users in EOS (" +
                       std::to_string(response.code) + "):\n" +
                       std::string(response.data.begin(), response.data.end())},
            pavlov::SearchUsersResponse{});
    return;
  }

  try {
    nlohmann::json json_response = nlohmann::json::parse(response.data);
    pavlov::SearchUsersResponse eos_response = json_response;
    std::move(on_done_callback)
        .Run(Result{Result::Status::kOk, ""}, std::move(eos_response));
  } catch (const std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << "() failed to parse response: " << e.what()
               << "\n"
               << std::string(response.data.begin(), response.data.end());
    std::move(on_done_callback)
        .Run(Result{Result::Status::kFailed,
                    "Failed to parse response from EOS:\n" +
                        std::string(e.what())},
             pavlov::SearchUsersResponse{});
  }
}

}  // namespace engine::backend
