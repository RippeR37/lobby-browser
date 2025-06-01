#include "engine/games/pavlov/pavlov_game.h"

#include <memory>
#include <string>

#include "base/barrier_callback.h"
#include "base/logging.h"
#include "base/net/resource_request.h"
#include "base/net/simple_url_loader.h"

#include "engine/backends/eos/eos_data.h"
#include "engine/backends/eos/eos_lobby_backend.h"
#include "engine/backends/steam/steam_subprocess_auth_backend.h"
#include "utils/strings.h"

namespace engine::game {

namespace {
// Options

// Enable computing server pings (requires per-server network requests)
const bool kEnableComputingServerPings = true;

// Steam auth
const int kPavlovSteamAppId = 555160;

// EOS Auth
const std::string kPavlovEosAuthInitToken =
    "Basic "
    "eHl6YTc4OTFSbmRPSUhwTWlacmtQNXFkRE5ZRHJQZ3Y6ZlE2djdDVFVEdFYxVVo4bjdFOWpYQn"
    "hMMEZHSXhXbEVlZkNYV2VYR2czZw==";
const std::string kPavlovEosDeploymentId = "e708e7885689412aa634bef12ec60023";
const std::string kPavlovEosNonce = "yA5lpRhWbECU8XQ8Xrb44w";

std::string MakePavlovServerMasterListUrl(const pavlov::PavlovFilters& request,
                                          const std::string& version) {
  const std::string kUrlPrefix =
      "https://prod-crossplay-pavlov-ms.vankrupt.net/servers/v2/list/";
  const std::string kUrlInfix = "/steam/0/0/0/";

  std::string game_modes = "ALL";
  if (!request.game_modes.AllEnabled()) {
    for (const auto& game_mode : request.game_modes.ToVec()) {
      if (!game_modes.empty()) {
        game_modes += ",";
      }
      game_modes += game_mode;
    }
  }

  return kUrlPrefix + version + kUrlInfix + game_modes;
}

backend::eos::SearchLobbiesRequest MakeEosRequest(
    const pavlov::PavlovFilters& filters,
    int64_t max_results = 200,
    int64_t min_current_players = 1) {
  backend::eos::SearchLobbiesRequest eos_request;
  eos_request.max_results = max_results;
  eos_request.min_current_players = min_current_players;

  if (!filters.game_modes.AllEnabled()) {
    if (auto game_modes = filters.game_modes.ToVec(); !game_modes.empty()) {
      eos_request.criteria.push_back(backend::eos::SearchLobbiesCriteria{
          "attributes.GAMETYPE_s",
          backend::eos::CriteriaOperator::ANY_OF,
          std::move(game_modes),
      });
    }
  }

  if (!filters.host_modes.regions.AllEnabled()) {
    if (auto regions = filters.host_modes.regions.ToVec(); !regions.empty()) {
      eos_request.criteria.push_back(backend::eos::SearchLobbiesCriteria{
          "attributes.REGION_s",
          backend::eos::CriteriaOperator::ANY_OF,
          std::move(regions),
      });
    }
  }

  if (!filters.host_modes.crossplay) {
    eos_request.criteria.push_back(backend::eos::SearchLobbiesCriteria{
        "attributes.CROSSPLATFORM_s",
        backend::eos::CriteriaOperator::EQUAL,
        "0",
    });
  }

  // EOS API requires at least 1 filter criteria for this API endpoint, so let's
  // add one that will always be true.
  if (eos_request.criteria.empty()) {
    eos_request.criteria.push_back(backend::eos::SearchLobbiesCriteria{
        "attributes.VERSION_s",
        backend::eos::CriteriaOperator::NOT_EQUAL,
        "0.0",
    });
  }

  return eos_request;
}

model::GameSearchResults FilterModelResultsFunction(
    const model::GameSearchResults& unfiltered_results,
    const model::GameFilters& model_filters) {
  const auto filters = pavlov::FromModel(model_filters);
  model::GameSearchResults filtered_results;

  for (const auto& result : unfiltered_results) {
    // Only filter via `filters.other`, rest is filtered on the backend side
    if (filters.others.hide_empty || filters.others.hide_full) {
      if (result.result_fields.size() > 4) {
        int players = 0, max_players = 0;
        if (std::sscanf(result.result_fields[4].c_str(), "%d/%d", &players,
                        &max_players) == 2) {
          if (filters.others.hide_empty && players == 0) {
            continue;
          }
          if (filters.others.hide_full && players == max_players) {
            continue;
          }
        }
      }
    }

    if (filters.others.hide_locked && result.result_fields.size() > 7) {
      if (result.result_fields[7].find("🔒") != std::string::npos) {
        continue;
      }
    }

    if (filters.others.only_compatible && result.result_fields.size() > 7) {
      if (result.result_fields[6] != "PC VR") {
        if (result.result_fields[7].find("⫘") == std::string::npos) {
          continue;
        }
      }
    }

    filtered_results.emplace_back(result);
  }

  return filtered_results;
}
}  // namespace

PavlovGame::PavlovGame(SetStatusTextCallback set_status_text,
                       ReportMessageCallback report_message,
                       nlohmann::json game_config,
                       std::string steam_auth_backend_command)
    : BaseGame(std::move(set_status_text), std::move(report_message)),
      config_(),
      lobby_backend_(nullptr),
      weak_factory_(this) {
  weak_this_ = weak_factory_.GetWeakPtr();

  if (!game_config.is_null()) {
    try {
      config_ = game_config;
    } catch (const std::exception& e) {
      LOG(ERROR) << __FUNCTION__
                 << "() failed to load game config: " << e.what();
    }
  }

  lobby_backend_ = std::make_unique<backend::EosLobbyBackend>(
      kPavlovEosAuthInitToken, kPavlovEosDeploymentId, kPavlovEosNonce,
      std::make_unique<backend::SteamSubprocessAuthBackend>(
          kPavlovSteamAppId,
          backend::SteamAuthBackend::AuthType::kEncryptedAppTicket,
          steam_auth_backend_command));
}

PavlovGame::~PavlovGame() = default;

model::Game PavlovGame::GetModel() const {
  return model::Game{
      "Pavlov",
      "IDI_ICON_PAVLOV",
      ToModel(config_.filters),
      model::GameResultsFormat{
          {
              model::GameResultsColumnFormat{
                  "Id",
                  false,
                  0,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Game Mode",
                  true,
                  100,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
                  true,
              },
              model::GameResultsColumnFormat{
                  "Owner",
                  true,
                  170,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
                  true,
              },
              model::GameResultsColumnFormat{
                  "Map",
                  true,
                  160,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
                  true,
              },
              model::GameResultsColumnFormat{
                  "Players",
                  true,
                  55,
                  model::GameResultsColumnAlignment::kRight,
                  model::GameResultsColumnOrdering::kNumberDivNumber,
                  true,
              },
              model::GameResultsColumnFormat{
                  "Region",
                  true,
                  80,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kPingOrString,
              },
              model::GameResultsColumnFormat{
                  "Platform",
                  true,
                  65,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Flags",
                  true,
                  35,
                  model::GameResultsColumnAlignment::kRight,
                  model::GameResultsColumnOrdering::kString,
              },
          },
          false,
      },
      base::BindRepeating(&FilterModelResultsFunction),
  };
}

nlohmann::json PavlovGame::UpdateConfig(model::GameFilters search_filters) {
  config_.filters = pavlov::FromModel(std::move(search_filters));
  return config_;
}

void PavlovGame::SearchLobbiesAndServers(
    model::SearchRequest request,
    base::OnceCallback<void(model::SearchResponse)> on_done_callback) {
  auto request_filters = pavlov::FromModel(std::move(request.search_filters));
  if (!request_filters.host_modes.lobby && !request_filters.host_modes.server) {
    request_filters.host_modes.lobby = true;
    request_filters.host_modes.server = true;
  }

  SetStatusText("Waiting for results...");

  const size_t barrier_count =
      request_filters.host_modes.lobby + request_filters.host_modes.server;
  auto response_callback = base::BarrierCallback<pavlov::PavlovSearchResponse>(
      barrier_count,
      base::BindOnce(&PavlovGame::CombineSearchResponses)
          .Then(base::BindOnce(&PavlovGame::StoreAndConvertSearchResults,
                               weak_this_, std::move(on_done_callback))));

  if (request_filters.host_modes.lobby) {
    auto eos_request = MakeEosRequest(request_filters);
    lobby_backend_->SearchLobbies(
        std::move(eos_request), base::BindOnce(&PavlovGame::OnSearchLobbiesDone,
                                               weak_this_, response_callback));
  }
  if (request_filters.host_modes.server) {
    if (config_.game_version.empty()) {
      // Do a EOS lobby search and take version out of the results, as it should
      // contain current version
      lobby_backend_->SearchLobbies(
          MakeEosRequest({}, 1),
          base::BindOnce(&PavlovGame::UpdateGameVersionFromLobbyResults,
                         weak_this_)
              .Then(base::BindOnce(&PavlovGame::RequestServerList, weak_this_,
                                   request_filters, response_callback)));
      return;
    }

    RequestServerList(request_filters, response_callback);
  }
}

void PavlovGame::GetServerLobbyDetails(
    model::SearchDetailsRequest request,
    base::OnceCallback<void(model::SearchDetailsResponse)> on_done_callback) {
  (void)request;

  // Unsupported
  std::move(on_done_callback).Run({});
}

// static
pavlov::PavlovSearchResponse PavlovGame::CombineSearchResponses(
    std::vector<pavlov::PavlovSearchResponse> responses) {
  if (responses.size() == 1) {
    return responses.front();
  }

  auto combined_success = true;
  std::string combined_errors;
  std::vector<pavlov::PavlovLobbyServer> combined_results;

  for (const auto& response : responses) {
    combined_success = combined_success && response.success;
    if (!response.error.empty()) {
      if (!combined_errors.empty()) {
        combined_errors += "\n\n";
      }
      combined_errors += response.error;
    }
    combined_results.insert(combined_results.end(), response.results.begin(),
                            response.results.end());
  }

  return pavlov::PavlovSearchResponse{combined_success,
                                      std::move(combined_errors),
                                      std::move(combined_results)};
}

void PavlovGame::UpdateGameVersionFromLobbyResults(
    backend::Result result,
    backend::eos::SearchLobbiesResponse response) {
  if (result.status != backend::Result::Status::kOk &&
      response.sessions.empty()) {
    LOG(WARNING) << "Failed to update known Pavlov game version";
    ReportMessage(Presenter::MessageType::kWarning,
                  "Failed to deduce Pavlov's current game version, searching "
                  "servers might not work",
                  result.error);
    return;
  }

  UpdateGameVersionFromLobbySession(response.sessions.front());
}

void PavlovGame::UpdateGameVersionFromLobbySession(
    const backend::eos::SearchLobbiesSession& session) {
  auto version_attr = session.attributes.find("VERSION_s");
  if (version_attr == session.attributes.end()) {
    return;
  }

  const auto& new_ver_str = version_attr->second;

  if (config_.game_version.empty()) {
    LOG(INFO) << __FUNCTION__ << "() Setting version to " << new_ver_str;
    config_.game_version = new_ver_str;
  } else {
    const auto current_ver = util::SplitString(config_.game_version, '.');
    const auto new_ver = util::SplitString(new_ver_str, '.');

    if (new_ver > current_ver) {
      LOG(INFO) << __FUNCTION__ << "() Updating version from "
                << config_.game_version << " to " << new_ver_str;

      config_.game_version = new_ver_str;
    }
  }
}

void PavlovGame::RequestServerList(
    pavlov::PavlovFilters request,
    base::OnceCallback<void(pavlov::PavlovSearchResponse)> on_done_callback) {
  DLOG_IF(ERROR, config_.game_version.empty())
      << "Unexpected invalid game version";

  const auto kMaxResponseSize = 5 * 1024 * 1024;
  const auto url = MakePavlovServerMasterListUrl(request, config_.game_version);

  base::net::SimpleUrlLoader::DownloadLimited(
      base::net::ResourceRequest{url}, kMaxResponseSize,
      base::BindOnce(&PavlovGame::OnSearchServersDone, weak_this_,
                     std::move(on_done_callback)));
}

void PavlovGame::OnSearchLobbiesDone(
    base::OnceCallback<void(pavlov::PavlovSearchResponse)> on_done_callback,
    backend::Result result,
    backend::eos::SearchLobbiesResponse response) {
  SetStatusText("Received lobby results, waiting for rest.");

  if (result.status != backend::Result::Status::kOk) {
    std::move(on_done_callback)
        .Run(pavlov::PavlovSearchResponse{false, result.error, {}});
    return;
  }

  if (!response.sessions.empty()) {
    UpdateGameVersionFromLobbySession(response.sessions.front());
  }

  std::vector<pavlov::PavlovLobbyServer> results;
  for (auto& lobby : response.sessions) {
    const bool pin_locked = !lobby.attributes["PINPROTECTED_s"].empty();
    const bool crossplay = lobby.attributes["CROSSPLATFORM_s"] == "1";

    auto platform = [](const std::string& platform_id)
        -> std::optional<pavlov::PavlovPlatform> {
      if (platform_id == "0") {
        return pavlov::PavlovPlatform::kPCVR;
      }
      if (platform_id == "1") {
        return pavlov::PavlovPlatform::kPSVR2;
      }
      return std::nullopt;
    };

    results.push_back(pavlov::PavlovLobbyServer{
        lobby.id,
        lobby.attributes["OWNERNAME_s"],
        lobby.total_players,
        lobby.settings.max_public_players,
        lobby.attributes["GAMELABEL_s"],
        lobby.attributes["MAP_s"],
        lobby.attributes["MAPLABEL_s"],
        crossplay,
        pin_locked,
        platform(lobby.attributes["PLATFORM_s"]),
        lobby.attributes["STATE_s"],
        lobby.attributes["REGION_s"],

        // Server-specific data (ip & port)
        "",
        -1,
    });
  }

  std::move(on_done_callback)
      .Run(pavlov::PavlovSearchResponse{true, "", std::move(results)});
}

void PavlovGame::OnSearchServersDone(
    base::OnceCallback<void(pavlov::PavlovSearchResponse)> on_done_callback,
    base::net::ResourceResponse response) {
  SetStatusText("Received server results, waiting for rest.");

  if (response.result == base::net::Result::kAborted) {
    std::move(on_done_callback).Run(pavlov::PavlovSearchResponse{true, "", {}});
    return;
  }

  if (response.result != base::net::Result::kOk) {
    std::move(on_done_callback)
        .Run(pavlov::PavlovSearchResponse{
            false,
            "Failed to query server list with code: " +
                std::to_string(response.code),
            {},
        });
    return;
  }

  std::vector<pavlov::PavlovLobbyServer> results;
  try {
    nlohmann::json json_response = nlohmann::json::parse(response.data);
    pavlov::PavlovServerListResponse server_list_response = json_response;

    for (const auto& server : server_list_response.servers) {
      results.emplace_back(pavlov::PavlovLobbyServer{
          server.ip + ":" + std::to_string(server.port),
          server.name,
          server.slots,
          server.max_slots,
          server.game_mode_label,
          server.map_id,
          server.map_label,
          false,  // PC servers are not crossplatform
          server.password_protected,
          pavlov::PavlovPlatform::kPCVR,
          "",     // state
          "?ms",  // region
          server.ip,
          server.port,
      });
    }

  } catch (const std::exception& e) {
    std::move(on_done_callback)
        .Run(pavlov::PavlovSearchResponse{
            false,
            "Failed to parse server list: " + std::string(e.what()),
            {},
        });
  }

  auto search_response = pavlov::PavlovSearchResponse{true, "", results};

  if (kEnableComputingServerPings) {
    auto got_ping_callback =
        base::BarrierCallback<std::pair<std::string, base::TimeDelta>>(
            results.size(),
            base::BindOnce(&PavlovGame::OnAllServersPingsReceived, weak_this_,
                           std::move(on_done_callback),
                           std::move(search_response)));

    for (const auto& result : results) {
      const auto kResponseMaxSize = 16 * 1024;
      base::net::SimpleUrlLoader::DownloadLimited(
          base::net::ResourceRequest{result.ip + ":" +
                                     std::to_string(result.port)}
              .WithFollowRedirects(false)
              .WithHeadersOnly()
              .WithTimeout(base::Milliseconds(999)),
          kResponseMaxSize,
          base::BindOnce(&PavlovGame::OnServerPingReceived, weak_this_,
                         got_ping_callback, result.id));
    }
  } else {
    std::move(on_done_callback).Run(std::move(search_response));
  }
}

void PavlovGame::OnServerPingReceived(
    base::OnceCallback<void(std::pair<std::string, base::TimeDelta>)>
        on_done_callback,
    std::string result_id,
    base::net::ResourceResponse response) {
  auto ping = base::Milliseconds(999);
  if (response.result == base::net::Result::kOk) {
    ping = response.timing_connect - response.timing_queue;
  }
  std::move(on_done_callback).Run(std::make_pair(std::move(result_id), ping));
}

void PavlovGame::OnAllServersPingsReceived(
    base::OnceCallback<void(pavlov::PavlovSearchResponse)> on_done_callback,
    pavlov::PavlovSearchResponse partial_response,
    std::vector<std::pair<std::string, base::TimeDelta>> id_to_ping_vec) {
  std::unordered_map<std::string, base::TimeDelta> id_to_ping{
      id_to_ping_vec.begin(), id_to_ping_vec.end()};

  for (auto& result : partial_response.results) {
    auto result_ping_it = id_to_ping.find(result.id);
    if (result_ping_it != id_to_ping.end()) {
      result.region =
          std::to_string(result_ping_it->second.InMilliseconds()) + "ms";
    }
  }

  std::move(on_done_callback).Run(std::move(partial_response));
}

void PavlovGame::StoreAndConvertSearchResults(
    base::OnceCallback<void(model::SearchResponse)> on_done_callback,
    pavlov::PavlovSearchResponse response) {
  SetStatusText("Received all results.");

  if (response.success) {
    last_search_results_ = std::move(response.results);
  } else {
    ReportMessage(Presenter::MessageType::kError,
                  "Failed to search Pavlov server/lobbies", response.error);
  }

  model::SearchResponse model_response;
  model_response.result =
      response.success ? model::SearchResult::kOk : model::SearchResult::kError;
  model_response.error = response.error;

  if (last_search_results_) {
    for (const auto& result : *last_search_results_) {
      std::string flags;
      if (result.locked) {
        flags += "🔒";
      }
      if (result.crossplatform) {
        flags += "⫘";
      } else if (!flags.empty()) {
        flags += "     ";
      }

      const auto* platform_str =
          [](std::optional<pavlov::PavlovPlatform> platform) {
            if (!platform) {
              return "Unknown";
            }
            switch (*platform) {
              case pavlov::PavlovPlatform::kPCVR:
                return "PC VR";
              case pavlov::PavlovPlatform::kPSVR2:
                return "PS VR2";
            }
          }(result.platform);

      model_response.results.emplace_back(model::GameServerLobbyResult{{
          result.id,
          result.gamemode,
          result.name_owner,
          result.map_label,
          std::to_string(result.players) + "/" +
              std::to_string(result.max_players),
          result.region,
          platform_str,
          flags,
      }});
    }
  }

  std::move(on_done_callback).Run(std::move(model_response));
}

}  // namespace engine::game
