#include "engine/backends/eos/eos_lobby_backend.h"

#include "base/barrier_callback.h"
#include "base/net/resource_request.h"
#include "base/net/simple_url_loader.h"
#include "nlohmann/json.hpp"

#include "engine/backends/eos/eos_data_serialize.h"
#include "engine/backends/eos/eos_lobby_connector.h"
#include "utils/vectors.h"

namespace engine::backend {

namespace {
const auto* kAuthUrl = "https://api.epicgames.dev/auth/v1/oauth/token";
const auto kAuthTimeout = base::Seconds(10);
const auto kSearchLobbiesTimeout = base::Seconds(15);
const auto* kFetchUsersInfoUrl =
    "https://api.epicgames.dev/user/v9/product-users/search";
const auto kFetchUsersInfoTimeout = base::Seconds(15);

std::string GetLobbyUrl(const std::string& eos_deployment_id) {
  return "https://api.epicgames.dev/lobby/v1/" + eos_deployment_id +
         "/lobbies/filter";
}
}  // namespace

EosLobbyBackend::EosLobbyBackend(
    std::string auth_init_token,
    std::string deployment_id,
    std::string nonce,
    std::unique_ptr<SteamAuthBackend> steam_auth_backend)
    : auth_init_token_(std::move(auth_init_token)),
      deployment_id_(std::move(deployment_id)),
      nonce_(std::move(nonce)),
      steam_auth_backend_(std::move(steam_auth_backend)),
      pending_auth_response_(false),
      weak_factory_(this) {
  DCHECK(steam_auth_backend_);
  weak_this_ = weak_factory_.GetWeakPtr();
}

EosLobbyBackend::~EosLobbyBackend() = default;

void EosLobbyBackend::SearchLobbies(
    eos::SearchLobbiesRequest request,
    base::OnceCallback<void(Result, eos::SearchLobbiesResponse)>
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

  if (!access_token_.IsValid()) {
    requests_pending_auth_.emplace_back(
        base::BindOnce(&EosLobbyBackend::DoSearchLobbies, weak_this_,
                       json_request.dump(), std::move(on_done_callback)));
    if (!pending_auth_response_) {
      StartAuthenticateViaSteam();
    }
    return;
  }

  DoSearchLobbies(json_request.dump(), std::move(on_done_callback), {});
}

void EosLobbyBackend::FetchUsersInfo(
    eos::FetchUsersInfoRequest request,
    base::OnceCallback<void(Result, eos::FetchUsersInfoResponse)>
        on_done_callback) {
  // EOS backend only supports fetching up to 128 users at once, so let's split
  // request if need metadata for more.
  const auto kMaxUsersPerRequest = 128;
  auto request_chunks_data =
      util::ToChunks(request.product_user_ids, kMaxUsersPerRequest);

  std::vector<std::string> serialized_requests;
  for (auto& request_chunk_data : request_chunks_data) {
    eos::FetchUsersInfoRequest request_chunk{request_chunk_data};

    try {
      nlohmann::json json_request = request_chunk;
      serialized_requests.push_back(json_request.dump());
    } catch (const std::exception& e) {
      LOG(ERROR) << __FUNCTION__
                 << "() failed to serialize request: " << e.what();
      std::move(on_done_callback)
          .Run(Result{Result::Status::kFailed,
                      "Failed to serialize request: " + std::string(e.what())},
               {});
      return;
    }
  }

  if (!access_token_.IsValid()) {
    requests_pending_auth_.emplace_back(base::BindOnce(
        &EosLobbyBackend::DoFetchUsersInfo, weak_this_,
        std::move(serialized_requests), std::move(on_done_callback)));
    if (!pending_auth_response_) {
      StartAuthenticateViaSteam();
    }
    return;
  }

  DoFetchUsersInfo(std::move(serialized_requests), std::move(on_done_callback),
                   {});
}

model::LobbyConnectorCreateCallback
EosLobbyBackend::GetLobbyConnectorCreateCallback() {
  return base::BindRepeating(
      [](std::string deployment_id, AuthToken<std::string> access_token,
         std::string lobby_id,
         base::RepeatingCallback<void(std::string)> status_update_cb,
         base::RepeatingCallback<void(int)> progress_update_cb,
         base::OnceCallback<void(bool)> on_done_callback)
          -> std::unique_ptr<model::LobbyConnector> {
        return std::make_unique<EosLobbyConnector>(
            std::move(deployment_id), std::move(access_token),
            std::move(lobby_id), std::move(status_update_cb),
            std::move(progress_update_cb), std::move(on_done_callback));
      },
      deployment_id_, access_token_);
}

void EosLobbyBackend::StartAuthenticateViaSteam() {
  pending_auth_response_ = true;

  steam_auth_backend_->Authenticate(
      base::BindOnce(&EosLobbyBackend::AuthenticateWithSteam, weak_this_));
}

void EosLobbyBackend::AuthenticateWithSteam(model::AuthResult auth_result) {
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

  LOG(INFO) << __FUNCTION__ << "() authenticated in Steam as "
            << auth_result.user_name;

  const auto kMaxResponseSize = 1 * 1024 * 1024;
  base::net::SimpleUrlLoader::DownloadLimited(
      base::net::ResourceRequest{kAuthUrl}
          .WithHeaders({
              std::string("Authorization: ") + auth_init_token_,
              std::string("Content-Type: application/x-www-form-urlencoded"),
          })
          .WithPostData(
              std::string("grant_type=external_auth") +
              std::string("&external_auth_type=steam_encrypted_appticket") +
              std::string("&deployment_id=") + deployment_id_ +
              std::string("&nonce=") + nonce_ +
              std::string("&external_auth_token=") + auth_result.auth_token)
          .WithTimeout(kAuthTimeout),
      kMaxResponseSize,
      base::BindOnce(&EosLobbyBackend::OnEosAuthResponse, weak_this_));
}

void EosLobbyBackend::OnEosAuthResponse(base::net::ResourceResponse response) {
  pending_auth_response_ = false;

  if (response.result != base::net::Result::kOk) {
    LOG(ERROR) << __FUNCTION__ << "() failed to authenticate with EOS";
    ProcessPendingRequests(Result{
        Result::Status::kAuthFailed,
        "Failed to authenticate with EOS:\n" +
            std::string(response.data.begin(), response.data.end()),
    });
    return;
  }

  nlohmann::json json = nlohmann::json::parse(response.data);
  if (json.contains("access_token")) {
    const auto& access_token = json["access_token"];
    if (access_token.is_string()) {
      // Detect auth TTL and invalidate it a bit earlier to reauthenticate
      // before requests will start to fail. By default assume sub-1h TTL.
      base::TimeDelta auth_ttl = base::Minutes(59);
      if (json.contains("expires_in") &&
          json["expires_in"].is_number_integer()) {
        auth_ttl = base::Seconds(json["expires_in"].get<int64_t>());
        if (auth_ttl > base::Minutes(10)) {
          auth_ttl -= base::Minutes(1);
        }
      }

      access_token_ =
          AuthToken<std::string>(access_token.get<std::string>(), auth_ttl);

      if (json.contains("id_token") && json["id_token"].is_string()) {
        id_token_ = AuthToken<std::string>(json["id_token"].get<std::string>(),
                                           auth_ttl);
      }
    }
  }

  ProcessPendingRequests({});
}

void EosLobbyBackend::ProcessPendingRequests(
    std::optional<Result> error_result) {
  DCHECK(!pending_auth_response_);

  auto requests = std::move(requests_pending_auth_);
  requests_pending_auth_.clear();

  for (auto& request : requests) {
    std::move(request).Run(error_result);
  }
}

void EosLobbyBackend::DoSearchLobbies(
    std::string json_request,
    base::OnceCallback<void(Result, eos::SearchLobbiesResponse)>
        on_done_callback,
    std::optional<Result> error_result) {
  if (!access_token_.IsValid()) {
    std::move(on_done_callback)
        .Run(error_result.value_or(
                 Result{Result::Status::kAuthFailed, "Failed to authenticate"}),
             eos::SearchLobbiesResponse{});
    return;
  }

  const auto kMaxResponseSize = 5 * 1024 * 1024;
  base::net::SimpleUrlLoader::DownloadLimited(
      base::net::ResourceRequest{GetLobbyUrl(deployment_id_)}
          .WithHeaders({
              std::string("Authorization: Bearer ") + *access_token_.GetToken(),
              std::string("Content-Type: application/json"),
          })
          .WithPostData(std::move(json_request))
          .WithTimeout(kSearchLobbiesTimeout),
      kMaxResponseSize,
      base::BindOnce(&EosLobbyBackend::OnSearchLobbiesResponse, weak_this_,
                     std::move(on_done_callback)));
}

void EosLobbyBackend::DoFetchUsersInfo(
    std::vector<std::string> json_requests,
    base::OnceCallback<void(Result, eos::FetchUsersInfoResponse)>
        on_done_callback,
    std::optional<Result> error_result) {
  if (!access_token_.IsValid()) {
    std::move(on_done_callback)
        .Run(error_result.value_or(
                 Result{Result::Status::kAuthFailed, "Failed to authenticate"}),
             eos::FetchUsersInfoResponse{});
    return;
  }

  auto chunk_response_callback =
      base::BarrierCallback<base::net::ResourceResponse>(
          json_requests.size(),
          base::BindOnce(&EosLobbyBackend::OnFetchUsersInfoResponse, weak_this_,
                         std::move(on_done_callback)));

  for (auto& json_request : json_requests) {
    const auto kMaxResponseSize = 2 * 1024 * 1024;
    base::net::SimpleUrlLoader::DownloadLimited(
        base::net::ResourceRequest{kFetchUsersInfoUrl}
            .WithHeaders({
                std::string("Authorization: Bearer ") +
                    *access_token_.GetToken(),
                std::string("Content-Type: application/json"),
            })
            .WithPostData(std::move(json_request))
            .WithTimeout(kFetchUsersInfoTimeout),
        kMaxResponseSize, chunk_response_callback);
  }
}

void EosLobbyBackend::OnSearchLobbiesResponse(
    base::OnceCallback<void(Result, eos::SearchLobbiesResponse)>
        on_done_callback,
    base::net::ResourceResponse response) {
  if (response.result != base::net::Result::kOk) {
    std::move(on_done_callback)
        .Run(
            Result{Result::Status::kFailed,
                   "Failed to search lobbies in EOS (" +
                       std::to_string(response.code) + "):\n" +
                       std::string(response.data.begin(), response.data.end())},
            eos::SearchLobbiesResponse{});
    return;
  }

  try {
    nlohmann::json json_response = nlohmann::json::parse(response.data);
    eos::SearchLobbiesResponse eos_response = json_response;
    eos_response.todo_auth_token = access_token_.GetToken().value_or("");
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
             eos::SearchLobbiesResponse{});
  }
}

void EosLobbyBackend::OnFetchUsersInfoResponse(
    base::OnceCallback<void(Result, eos::FetchUsersInfoResponse)>
        on_done_callback,
    std::vector<base::net::ResourceResponse> responses) {
  eos::FetchUsersInfoResponse combined_response;

  for (auto& response : responses) {
    if (response.result != base::net::Result::kOk) {
      continue;
    }

    try {
      nlohmann::json json_response = nlohmann::json::parse(response.data);
      eos::FetchUsersInfoResponse eos_response = json_response;
      combined_response.product_users.insert(eos_response.product_users.begin(),
                                             eos_response.product_users.end());
    } catch (const std::exception& e) {
      LOG(ERROR) << __FUNCTION__ << "() failed to parse response: " << e.what();
    }
  }

  std::move(on_done_callback)
      .Run(Result{Result::Status::kOk, ""}, std::move(combined_response));
}

}  // namespace engine::backend
