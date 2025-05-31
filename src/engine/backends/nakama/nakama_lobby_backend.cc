#include "engine/backends/nakama/nakama_lobby_backend.h"

#include "base/net/resource_request.h"
#include "base/net/simple_url_loader.h"
#include "base/time/time_delta.h"
#include "nlohmann/json.hpp"

#include "engine/backends/nakama/nakama_data.h"
#include "engine/backends/nakama/nakama_data_serialize.h"

namespace engine::backend {

namespace {
const auto* kAuthUrlPath = "v2/account/authenticate/steam?create=false";
const auto kAuthTimeout = base::Seconds(10);
const auto kAuthTokenTtl = base::Hours(1);
const auto* kSearchLobbyUrlPath = "v2/rpc/apeplugin-getadvertisedcustomlobby";
const auto kSearchLobbyTimeout = base::Seconds(15);
}  // namespace

NakamaLobbyBackend::NakamaLobbyBackend(
    std::string server_url,
    std::string auth_init_token,
    std::unique_ptr<SteamAuthBackend> steam_auth_backend)
    : server_url_(std::move(server_url)),
      auth_init_token_(std::move(auth_init_token)),
      steam_auth_backend_(std::move(steam_auth_backend)),
      pending_auth_response_(false),
      weak_factory_(this) {
  weak_this_ = weak_factory_.GetWeakPtr();
}

NakamaLobbyBackend::~NakamaLobbyBackend() = default;

void NakamaLobbyBackend::SearchLobbies(
    nakama::LobbySearchRequest request,
    base::OnceCallback<void(Result, nakama::LobbySearchResponse)>
        on_done_callback) {
  if (!auth_token_.IsValid()) {
    requests_pending_auth_.emplace_back(
        base::BindOnce(&NakamaLobbyBackend::DoSearchLobbies, weak_this_,
                       std::move(request), std::move(on_done_callback)));
    if (!pending_auth_response_) {
      StartAuthenticateViaSteam();
    }
    return;
  }

  DoSearchLobbies(std::move(request), std::move(on_done_callback), {});
}

void NakamaLobbyBackend::StartAuthenticateViaSteam() {
  pending_auth_response_ = true;

  steam_auth_backend_->Authenticate(
      base::BindOnce(&NakamaLobbyBackend::AuthenticateWithSteam, weak_this_));
}

void NakamaLobbyBackend::AuthenticateWithSteam(model::AuthResult auth_result) {
  if (!auth_result.success) {
    LOG(ERROR) << __FUNCTION__
               << "() authentication with Steam failed: " << auth_result.error;
    pending_auth_response_ = false;
    ProcessPendingRequests(Result{
        Result::Status::kAuthFailed,
        "Failed to authenticate with Steam:\n" + auth_result.error,
    });
    return;
  }

  auto auth_request = nakama::AuthRequest{auth_result.auth_token};

  const auto kMaxResponseSize = 1 * 1024 * 1024;
  base::net::SimpleUrlLoader::DownloadLimited(
      base::net::ResourceRequest{server_url_ + kAuthUrlPath}
          .WithHeaders({
              "Authorization: Basic " + auth_init_token_,
              "Content-Type: application/json",
          })
          .WithPostData(nlohmann::json(auth_request).dump())
          .WithTimeout(kAuthTimeout),
      kMaxResponseSize,
      base::BindOnce(&NakamaLobbyBackend::OnNakamaAuthResponse, weak_this_));
}

void NakamaLobbyBackend::OnNakamaAuthResponse(
    base::net::ResourceResponse response) {
  pending_auth_response_ = false;

  if (response.result != base::net::Result::kOk) {
    LOG(ERROR) << __FUNCTION__ << "() failed to authenticate (" << response.code
               << ")";
    ProcessPendingRequests(Result{
        Result::Status::kAuthFailed,
        "Failed to authenticate with Nakama (" + std::to_string(response.code) +
            "):\n" + std::string(response.data.begin(), response.data.end()),
    });
    return;
  }

  try {
    auto json_response = nlohmann::json::parse(response.data);

    if (json_response.contains("error")) {
      LOG(ERROR) << __FUNCTION__
                 << "() error: " << json_response["error"].get<std::string>();
      ProcessPendingRequests(
          Result{Result::Status::kFailed,
                 "Failed to authenticate with Nakama (" +
                     std::to_string(response.code) + "):\n" +
                     json_response["error"].get<std::string>()});
      return;
    }

    nakama::AuthResponse response = json_response;
    auth_token_ = AuthToken<std::string>(response.token, kAuthTokenTtl);
  } catch (const std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << "() failed to parse authentication response";

    ProcessPendingRequests(Result{
        Result::Status::kFailed,
        "Failed to parse authentication response from Nakama (" +
            std::to_string(response.code) + "):\n" + std::string(e.what())});
    return;
  }

  ProcessPendingRequests({});
}

void NakamaLobbyBackend::ProcessPendingRequests(
    std::optional<Result> error_result) {
  DCHECK(!pending_auth_response_);

  auto requests = std::move(requests_pending_auth_);
  requests_pending_auth_.clear();

  for (auto& request : requests) {
    std::move(request).Run(error_result);
  }
}

void NakamaLobbyBackend::DoSearchLobbies(
    nakama::LobbySearchRequest request,
    base::OnceCallback<void(Result, nakama::LobbySearchResponse)>
        on_done_callback,
    std::optional<Result> error_result) {
  if (!auth_token_.IsValid()) {
    std::move(on_done_callback)
        .Run(error_result.value_or(
                 Result{Result::Status::kAuthFailed, "Failed to authenticate"}),
             nakama::LobbySearchResponse{});
    return;
  }

  // For some reason the Nakama client wraps JSON request in a string
  // (escapes ") and then parses and dumps such JSON again before sending it.
  // This replicates that behavior.
  auto wrapped_escaped_json_request =
      nlohmann::json(nlohmann::json(request).dump()).dump();

  const auto kMaxResponseSize = 1 * 1024 * 1024;
  base::net::SimpleUrlLoader::DownloadLimited(
      base::net::ResourceRequest{server_url_ + kSearchLobbyUrlPath}
          .WithHeaders({
              "Authorization: Bearer " + *auth_token_.GetToken(),
              "Content-Type: application/json",
              "Accept: application/json",
          })
          .WithPostData(std::move(wrapped_escaped_json_request))
          .WithTimeout(kSearchLobbyTimeout),
      kMaxResponseSize,
      base::BindOnce(&NakamaLobbyBackend::OnSearchLobbiesResponse, weak_this_,
                     std::move(on_done_callback)));
}

void NakamaLobbyBackend::OnSearchLobbiesResponse(
    base::OnceCallback<void(Result, nakama::LobbySearchResponse)>
        on_done_callback,
    base::net::ResourceResponse response) {
  if (response.result != base::net::Result::kOk) {
    std::move(on_done_callback)
        .Run(
            Result{Result::Status::kFailed,
                   "Failed to search lobbies in Nakama (" +
                       std::to_string(response.code) + "):\n" +
                       std::string(response.data.begin(), response.data.end())},
            nakama::LobbySearchResponse{});
    return;
  }

  try {
    auto json_response = nlohmann::json::parse(response.data);

    if (json_response.contains("error")) {
      LOG(ERROR) << __FUNCTION__
                 << "() error: " << json_response["error"].get<std::string>();
      std::move(on_done_callback)
          .Run(Result{Result::Status::kFailed,
                      "Failed to search lobbies in Nakama (" +
                          std::to_string(response.code) + "):\n" +
                          json_response["error"].get<std::string>()},
               nakama::LobbySearchResponse{});
      return;
    }

    if (json_response.contains("payload") &&
        json_response["payload"].is_string()) {
      json_response =
          nlohmann::json::parse(json_response["payload"].get<std::string>());
    }

    nakama::LobbySearchResponse response = json_response;
    std::move(on_done_callback)
        .Run(Result{Result::Status::kOk, ""}, std::move(response));
  } catch (const std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << "() response format error: " << e.what();
    std::move(on_done_callback)
        .Run(Result{Result::Status::kFailed,
                    "Failed to parse response from Nakama:\n" +
                        std::string(e.what())},
             nakama::LobbySearchResponse{});
  }
}

}  // namespace engine::backend
