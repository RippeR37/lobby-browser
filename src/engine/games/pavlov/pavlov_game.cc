#include "engine/games/pavlov/pavlov_game.h"

#include <format>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "base/barrier_callback.h"
#include "base/logging.h"
#include "base/net/resource_request.h"
#include "base/net/simple_url_loader.h"

#include "engine/backends/eos/eos_data.h"
#include "engine/backends/pavlov/pavlov_lobby_backend.h"
#include "utils/strings.h"

namespace engine::game {

namespace {
// Options
// Enable computing server pings (requires per-server network requests)
const bool kEnableComputingServerPings = true;

const auto kPavlovEosEmptyCriteria = backend::eos::SearchLobbiesCriteria{
    "attributes.VERSION_s",
    backend::eos::CriteriaOperator::NOT_EQUAL,
    "0.0",
};

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
    eos_request.criteria.push_back(kPavlovEosEmptyCriteria);
  }

  return eos_request;
}

model::GameSearchResults FilterModelResultsFunction(
    const model::GameSearchResults& unfiltered_results,
    const model::GameFilters& model_filters) {
  const auto filters = pavlov::FromModel(model_filters);
  model::GameSearchResults filtered_results;

  for (const auto& result : unfiltered_results.lobbies) {
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

    filtered_results.lobbies.emplace_back(result);
  }

  return filtered_results;
}

std::string ConvertTimestamp(const std::string input_time) {
  std::istringstream in{input_time};
  std::chrono::sys_seconds tp;
  in >> std::chrono::parse("%FT%T%EZ", tp);
  if (in.fail()) {
    return input_time;
  }
  std::chrono::zoned_time local_time{std::chrono::current_zone(), tp};
  return std::format("{0:%F %T}", local_time);
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

  lobby_backend_ = std::make_unique<backend::PavlovLobbyBackend>(
      std::move(steam_auth_backend_command));
}

PavlovGame::~PavlovGame() = default;

model::Game PavlovGame::GetModel() const {
  const bool has_favorites = !config_.favorite_players.empty();

  const auto players_favorites_width = has_favorites ? 20 : 0;

  return model::Game{
      "Pavlov",
      "IDI_ICON_PAVLOV",
      model::GameFeatures{
          .has_lobby_details = true,
          .search_players = true,
          .list_players = true,
          .connect_to_lobby = true,
      },
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
          {
              model::GameResultsColumnFormat{
                  "Lobby Id",
                  false,
                  0,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Player Id",
                  false,
                  0,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "",
                  true,
                  players_favorites_width,
                  model::GameResultsColumnAlignment::kCenter,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Platform",
                  true,
                  65,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Player Name",
                  true,
                  180 - players_favorites_width,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Lobby Owner",
                  true,
                  180,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Mode",
                  true,
                  180,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Map",
                  true,
                  180,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Players",
                  true,
                  55,
                  model::GameResultsColumnAlignment::kRight,
                  model::GameResultsColumnOrdering::kNumberDivNumber,
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

  // Request player list if asked for it
  if (request.players_callback) {
    const auto kMinPlayers = 1;
    const auto kMaxLobbies = 400;
    auto eos_request = backend::eos::SearchLobbiesRequest{
        {kPavlovEosEmptyCriteria}, kMinPlayers, kMaxLobbies};
    lobby_backend_->SearchLobbies(
        std::move(eos_request),
        base::BindOnce(&PavlovGame::OnSearchLobbiesForPlayersDone, weak_this_,
                       std::move(request.players_callback)));
  }
}

void PavlovGame::GetServerLobbyDetails(
    model::SearchDetailsRequest request,
    base::OnceCallback<void(model::SearchDetailsResponse)> on_done_callback) {
  if (pending_search_details_request_) {
    TryResolvePendingServerLobbyDetailsRequest(true);
  }

  std::optional<std::vector<pavlov::PavlovLobbyServer>::iterator> result_it;
  if (last_search_results_) {
    result_it =
        std::find_if(last_search_results_->begin(), last_search_results_->end(),
                     [&request](const pavlov::PavlovLobbyServer& result) {
                       return result.id == request.result_id;
                     });
    if (result_it == last_search_results_->end()) {
      result_it.reset();
    }
  }

  if (!result_it) {
    std::move(on_done_callback).Run({});
    return;
  }

  pending_search_details_request_ =
      std::make_pair(std::move(request), std::move(on_done_callback));

  TryResolvePendingServerLobbyDetailsRequest(false);
}

void PavlovGame::SearchUsers(
    model::SearchUsersRequest request,
    base::OnceCallback<void(model::SearchUsersResponse)> on_done_callback) {
  SetStatusText("Searching users...");

  lobby_backend_->SearchUsers(
      backend::pavlov::SearchUsersRequest{request.user_name},
      base::BindOnce(&PavlovGame::OnSearchUsersDone, weak_this_,
                     std::move(on_done_callback)));
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

  todo_auth_token_ = response.todo_auth_token;

  std::vector<pavlov::PavlovLobbyServer> results;
  for (auto& lobby : response.sessions) {
    results.push_back(pavlov::PavlovLobbyServer{lobby});
  }

  std::move(on_done_callback)
      .Run(pavlov::PavlovSearchResponse{true, "", std::move(results)});
}

void PavlovGame::OnSearchLobbiesForPlayersDone(
    base::OnceCallback<void(model::GamePlayersResults)> on_done_callback,
    backend::Result result,
    backend::eos::SearchLobbiesResponse response) {
  if (result.status != backend::Result::Status::kOk) {
    std::move(on_done_callback).Run(model::GamePlayersResults{});
    return;
  }

  if (!response.sessions.empty()) {
    UpdateGameVersionFromLobbySession(response.sessions.front());
  }

  std::vector<std::string> missing_data_ids;
  for (const auto& lobby : response.sessions) {
    for (const auto& player_id : lobby.public_players) {
      if (!players_data_store_.GetCachedDataFor(player_id)) {
        missing_data_ids.push_back(player_id);
      }
    }
  }

  if (!missing_data_ids.empty()) {
    lobby_backend_->FetchUsersInfo(
        backend::eos::FetchUsersInfoRequest{std::move(missing_data_ids)},
        base::BindOnce(&PavlovGame::OnFetchPlayersDataDone, weak_this_)
            .Then(base::BindOnce(&PavlovGame::OnSearchLobbyPlayersDone,
                                 weak_this_, std::move(on_done_callback),
                                 std::move(response))));
    return;
  }

  OnSearchLobbyPlayersDone(std::move(on_done_callback), std::move(response));
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
      results.emplace_back(pavlov::PavlovLobbyServer{server});
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

  bool update_player_data = false;

  if (response.success) {
    last_search_results_ = std::move(response.results);
    update_player_data = true;
  } else {
    ReportMessage(Presenter::MessageType::kError,
                  "Failed to search Pavlov server/lobbies", response.error);
  }

  model::SearchResponse model_response;
  model_response.create_lobby_connector =
      lobby_backend_->GetLobbyConnectorCreateCallback();
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

      model_response.results.lobbies.emplace_back(model::GameServerLobbyResult{{
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

  if (update_player_data) {
    std::vector<std::string> current_user_ids;
    for (const auto& result : *last_search_results_) {
      current_user_ids.insert(current_user_ids.end(), result.member_ids.begin(),
                              result.member_ids.end());
    }
    std::vector<std::string> unknown_user_ids =
        players_data_store_.OnNewUserIds(std::move(current_user_ids));
    if (!unknown_user_ids.empty()) {
      lobby_backend_->FetchUsersInfo(
          backend::eos::FetchUsersInfoRequest{std::move(unknown_user_ids)},
          base::BindOnce(&PavlovGame::OnFetchPlayersDataDone, weak_this_));
    }
  }
}

void PavlovGame::OnFetchPlayersDataDone(
    backend::Result result,
    backend::eos::FetchUsersInfoResponse response) {
  players_data_store_.ProcessNewUserData(std::move(response));
  if (pending_search_details_request_) {
    TryResolvePendingServerLobbyDetailsRequest(result.status ==
                                               backend::Result::Status::kOk);
  }
}

void PavlovGame::TryResolvePendingServerLobbyDetailsRequest(bool force) {
  if (!pending_search_details_request_ || !last_search_results_) {
    return;
  }

  const auto& request = pending_search_details_request_->first;
  const auto result_it =
      std::find_if(last_search_results_->begin(), last_search_results_->end(),
                   [&request](const pavlov::PavlovLobbyServer& result) {
                     return result.id == request.result_id;
                   });
  if (result_it == last_search_results_->end()) {
    std::move(pending_search_details_request_->second).Run({});
    return;
  }

  const auto& result = *result_it;
  model::SearchDetailsResponse response{true, {}};
  for (const auto& member_id : result.member_ids) {
    auto player_data = players_data_store_.GetCachedDataFor(member_id);

    if (player_data) {
      const auto* icon_url = [&]() {
        switch (*player_data->platform) {
          case pavlov::PavlovUserPlatform::kSteam:
            return "https://steamcommunity.com/favicon.ico";
          case pavlov::PavlovUserPlatform::kPlayStation:
            return "https://gmedia.playstation.com/is/image/SIEPDC/"
                   "ps-logo-favicon";
        }
      }();
      const auto profile_url = [&]() -> std::string {
        switch (*player_data->platform) {
          case pavlov::PavlovUserPlatform::kSteam:
            return "https://steamcommunity.com/profiles/" +
                   player_data->platform_id;
          default:
            return {};
        }
      }();
      response.members.emplace_back(model::SearchDetailsResponse::Member{
          player_data->platform_id.empty() ? member_id
                                           : player_data->platform_id,
          player_data->name,
          "http://prod.cdn.pavlov-vr.com/avatar/" + member_id + ".png",
          icon_url,
          profile_url,
      });
    } else if (pending_search_details_request_->first.wait_for_full_details &&
               !force) {
      // Don't have all the data which was request and isn't forcing it, so wait
      return;
    } else {
      response.all_members_known = false;
      response.members.emplace_back(model::SearchDetailsResponse::Member{
          member_id,
          "???",
          "http://prod.cdn.pavlov-vr.com/avatar/" + member_id + ".png",
          "",
          "",
      });
    }
  }

  auto on_done_callback = std::move(pending_search_details_request_->second);
  pending_search_details_request_.reset();
  std::move(on_done_callback).Run(std::move(response));
}

void PavlovGame::OnSearchUsersDone(
    base::OnceCallback<void(model::SearchUsersResponse)> on_done_callback,
    backend::Result result,
    backend::pavlov::SearchUsersResponse response) {
  SetStatusText("Received users data (" +
                std::to_string(response.players.size()) + ")");

  if (result.status != backend::Result::Status::kOk) {
    std::move(on_done_callback)
        .Run(model::SearchUsersResponse{
            model::SearchResult::kError, result.error, {}});
    return;
  }

  std::vector<std::string> missing_data_ids;
  for (auto& player : response.players) {
    if (!players_data_store_.GetCachedDataFor(player.id)) {
      missing_data_ids.push_back(player.id);
    }
  }

  if (!missing_data_ids.empty()) {
    // Fetch user data and then fill it
    lobby_backend_->FetchUsersInfo(
        backend::eos::FetchUsersInfoRequest{std::move(missing_data_ids)},
        base::BindOnce(&PavlovGame::OnFetchPlayersDataDone, weak_this_)
            .Then(base::BindOnce(&PavlovGame::OnSearchUsersWithDetailsDone,
                                 weak_this_, std::move(on_done_callback),
                                 std::move(response))));
    return;
  }

  OnSearchUsersWithDetailsDone(std::move(on_done_callback),
                               std::move(response));
}

void PavlovGame::OnSearchUsersWithDetailsDone(
    base::OnceCallback<void(model::SearchUsersResponse)> on_done_callback,
    backend::pavlov::SearchUsersResponse response) {
  std::vector<model::SearchUserEntry> results;
  for (auto& player : response.players) {
    auto last_seen = std::string{};
    if (auto data = players_data_store_.GetCachedDataFor(player.id)) {
      last_seen = ConvertTimestamp(data->last_login);
    }
    results.emplace_back(model::SearchUserEntry{
        player.display_name,
        {
            {"Platform", player.platform},
            {"Platform ID", player.platform_id},
            {"Last seen", last_seen},
            {"ID", player.id},
        },
    });
  }

  std::move(on_done_callback)
      .Run(model::SearchUsersResponse{
          model::SearchResult::kOk,
          "",
          std::move(results),
      });
}

void PavlovGame::OnSearchLobbyPlayersDone(
    base::OnceCallback<void(model::GamePlayersResults)> on_done_callback,
    backend::eos::SearchLobbiesResponse response) {
  model::GamePlayersResults result;

  for (const auto& lobby : response.sessions) {
    for (const auto& member_id : lobby.public_players) {
      auto player_name = member_id;
      auto player_platform = "";
      const auto player_data = players_data_store_.GetCachedDataFor(member_id);
      if (player_data) {
        player_name = player_data->name;
        switch (*player_data->platform) {
          case pavlov::PavlovUserPlatform::kSteam:
            player_platform = "Steam";
            break;
          case pavlov::PavlovUserPlatform::kPlayStation:
            player_platform = "PSN";
            break;
        }
      }

      const auto pavlov_lobby = pavlov::PavlovLobbyServer{lobby};

      const auto favorites_icon = std::string{"⭐"};
      const auto is_in_favorites =
          IsPlayerInFavorties(member_id) ||
          (player_data && IsPlayerInFavorties(player_data->platform_id));

      const auto icon = is_in_favorites ? favorites_icon : "";

      std::string flags;
      if (pavlov_lobby.locked) {
        flags += "🔒";
      }
      if (pavlov_lobby.crossplatform) {
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
          }(pavlov_lobby.platform);

      result.emplace_back(model::GamePlayersResult{{
          pavlov_lobby.id,
          member_id,
          icon,
          player_platform,
          player_name,
          pavlov_lobby.name_owner,
          pavlov_lobby.gamemode,
          pavlov_lobby.map_label,
          std::to_string(pavlov_lobby.players) + "/" +
              std::to_string(pavlov_lobby.max_players),
          pavlov_lobby.region,
          platform_str,
          flags,
      }});
    }
  }

  std::move(on_done_callback).Run(std::move(result));
}

bool PavlovGame::IsPlayerInFavorties(const std::string& player_id) const {
  return config_.favorite_players.count(player_id) > 0;
}

}  // namespace engine::game
