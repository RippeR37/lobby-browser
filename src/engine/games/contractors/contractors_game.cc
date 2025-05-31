#include "engine/games/contractors/contractors_game.h"

#include "engine/backends/steam/steam_subprocess_auth_backend.h"

namespace engine::game {

namespace {
const auto kContractorsSteamAppId = 963930;
const auto kNakamaServerUrl = "https://contractors.us-east1.nakamacloud.io/";
const auto kNakamaAuthInitToken = "cmxwOTFpM3lNRVA2Og==";
const auto kModIoServerUrl = "https://g-251.modapi.io";
const auto kModIoGameId = 251;
const auto kModIoApikey = "ef79251be7e046741cf0b350a7c4bf8a";

bool IsGameModeEnabled(contractors::ContractorsGameModeFilters filters,
                       const std::string& game_mode) {
  if (filters.AllEnabled()) {
    return true;
  }

  if (game_mode == "dm") {
    return filters.dm;
  }
  if (game_mode == "tdm") {
    return filters.tdm;
  }
  if (game_mode == "control") {
    return filters.control;
  }
  if (game_mode == "comp_control") {
    return filters.comp_control;
  }
  if (game_mode == "bh_ffa") {
    return filters.bh_ffa;
  }
  if (game_mode == "gungame_ffa") {
    return filters.gungame_ffa;
  }
  if (game_mode == "Rush") {
    return filters.rush;
  }
  if (game_mode == "Custom") {
    return filters.custom;
  }
  if (game_mode == "kc") {
    return filters.kc;
  }

  bool matched_gm = false;
  if (game_mode.find("domination") != std::string::npos ||
      game_mode.find("Domination") != std::string::npos) {
    matched_gm = true;
    if (filters.domination) {
      return true;
    }
  }
  if (game_mode.find("survival") != std::string::npos ||
      game_mode.find("Survival") != std::string::npos) {
    matched_gm = true;
    if (filters.survival) {
      return true;
    }
  }
  if (game_mode.find("zombie") != std::string::npos ||
      game_mode.find("Zombie") != std::string::npos) {
    matched_gm = true;
    if (filters.zombie) {
      return true;
    }
  }

  return !matched_gm && filters.other;
}

model::GameSearchResults FilterModelResultsFunction(
    const model::GameSearchResults& unfiltered_results,
    const model::GameFilters& model_filters) {
  const auto filters = contractors::FromModel(model_filters);
  model::GameSearchResults filtered_results;

  for (const auto& result : unfiltered_results) {
    if (result.result_fields.size() < 8) {
      continue;
    }

    const auto game_mode = result.result_fields[1];
    const auto game_type = result.result_fields[2];
    const auto players_str = result.result_fields[5];
    const auto region = result.result_fields[6];
    const auto flags = result.result_fields[7];

    // filters.game_modes
    if (!IsGameModeEnabled(filters.game_modes, game_mode)) {
      continue;
    }

    // filters.regions
    if (!filters.host_modes.regions.AllEnabled()) {
      if (region.substr(0, 2) == "us") {
        if (!filters.host_modes.regions.america) {
          continue;
        }
      } else if (region.substr(0, 2) == "eu") {
        if (!filters.host_modes.regions.europe) {
          continue;
        }
      } else if (!filters.host_modes.regions.other) {
        continue;
      }
    }

    if (filters.host_modes.casual || filters.host_modes.competitive ||
        filters.host_modes.other) {
      if (game_type == "casual") {
        if (!filters.host_modes.casual) {
          continue;
        }
      } else if (game_type == "competitive") {
        if (!filters.host_modes.competitive) {
          continue;
        }
      } else if (!filters.host_modes.other) {
        continue;
      }
    }

    // filters.other
    int players = 0, max_players = 0;
    if (std::sscanf(players_str.c_str(), "%d/%d", &players, &max_players) ==
        2) {
      if (filters.others.hide_full && players == max_players) {
        continue;
      }
    }
    if (filters.others.hide_locked) {
      if (flags.find("🔒") != std::string::npos) {
        continue;
      }
    }

    filtered_results.emplace_back(result);
  }

  return filtered_results;
}
}  // namespace

ContractorsGame::ContractorsGame(SetStatusTextCallback set_status_text,
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

  lobby_backend_ = std::make_unique<backend::NakamaLobbyBackend>(
      kNakamaServerUrl, kNakamaAuthInitToken,
      std::make_unique<backend::SteamSubprocessAuthBackend>(
          kContractorsSteamAppId,
          backend::SteamAuthBackend::AuthType::kAuthSessionTicket,
          std::move(steam_auth_backend_command)));
  mod_backend_ = std::make_unique<backend::ModIoModBackend>(
      kModIoServerUrl, kModIoGameId, kModIoApikey);

  // Start fetching immediately to speed up potential lobby search
  mod_backend_->FetchMods(
      base::BindOnce(&ContractorsGame::OnModsFetched, weak_this_));
}

ContractorsGame::~ContractorsGame() = default;

model::Game ContractorsGame::GetModel() const {
  return model::Game{
      "Contractors",
      "IDI_ICON_CONTRACTORS",
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
                  120,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
                  true,
              },
              model::GameResultsColumnFormat{
                  "Game Type",
                  true,
                  100,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Owner",
                  true,
                  140,
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
                  60,
                  model::GameResultsColumnAlignment::kRight,
                  model::GameResultsColumnOrdering::kNumberDivNumber,
                  true,
              },
              model::GameResultsColumnFormat{
                  "Region",
                  true,
                  80,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Flags",
                  true,
                  20,
                  model::GameResultsColumnAlignment::kRight,
                  model::GameResultsColumnOrdering::kString,
              },
          },
          false,
      },
      base::BindRepeating(&FilterModelResultsFunction),
  };
}

nlohmann::json ContractorsGame::UpdateConfig(
    model::GameFilters search_filters) {
  config_.filters = contractors::FromModel(std::move(search_filters));
  return config_;
}

void ContractorsGame::SearchLobbiesAndServers(
    model::SearchRequest request,
    base::OnceCallback<void(model::SearchResponse)> on_done_callback) {
  // It seems that the game doesn't use server-side filtering, so let's pass
  // clear request and do the filtering on the client side as well (in
  // `FilterModelResultsFunction`).
  (void)request;

  SetStatusText("Waiting for results...");
  lobby_backend_->SearchLobbies(
      backend::nakama::LobbySearchRequest{},
      base::BindOnce(&ContractorsGame::OnSearchLobbiesResponse, weak_this_,
                     std::move(on_done_callback)));
}

void ContractorsGame::GetServerLobbyDetails(
    model::SearchDetailsRequest request,
    base::OnceCallback<void(model::SearchDetailsResponse)> on_done_callback) {
  (void)request;

  // Unsupported
  std::move(on_done_callback).Run({});
}

void ContractorsGame::OnModsFetched() {
  DCHECK(mod_backend_->IsReady());

  auto callbacks = std::move(on_mods_fetched_callbacks_);
  on_mods_fetched_callbacks_.clear();

  for (auto& callback : callbacks) {
    std::move(callback).Run();
  }
}

void ContractorsGame::OnSearchLobbiesResponse(
    base::OnceCallback<void(model::SearchResponse)> on_done_callback,
    backend::Result result,
    backend::nakama::LobbySearchResponse response) {
  if (!mod_backend_->IsReady()) {
    // Defer processing until we fetch mods data to present full mod/map names
    on_mods_fetched_callbacks_.emplace_back(base::BindOnce(
        &ContractorsGame::OnSearchLobbiesResponse, weak_this_,
        std::move(on_done_callback), std::move(result), std::move(response)));
    return;
  }

  if (result.status != backend::Result::Status::kOk) {
    ReportMessage(Presenter::MessageType::kError,
                  "Failed to search Contractors lobbies", result.error);
  }

  model::GameSearchResults results;

  for (auto& result : response.listserver_results) {
    std::string flags;
    if (result.match_data.password_protected) {
      flags += "🔒";
    }

    auto game_mode = result.match_data.mode_tag;
    if (game_mode.find("gamemode_") == 0) {
      game_mode = game_mode.substr(std::size("gamemode_") - 1);
    }

    auto map = result.match_data.map_tag;
    if (map.find("map_") == 0) {
      map = map.substr(std::size("map_") - 1);
    }

    if (map.find("+0") != std::string::npos) {
      auto map_mod_id = map.substr(0, map.find("+0"));
      if (auto mod_data = mod_backend_->GetModData(std::stoll(map_mod_id))) {
        map = mod_data->name;
      }
    }

    results.emplace_back(model::GameServerLobbyResult{{
        result.lobby_id,
        game_mode,
        result.match_data.game_type,
        result.match_data.owner_name,
        map,
        std::to_string(result.match_data.current_player_num) + "/" +
            std::to_string(result.match_data.max_player_num),
        result.match_data.region,
        flags,
    }});
  }

  SetStatusText("Received all results.");

  const auto search_result = [&result]() {
    switch (result.status) {
      case backend::Result::Status::kOk:
        return model::SearchResult::kOk;
      case backend::Result::Status::kAuthFailed:
      case backend::Result::Status::kFailed:
        return model::SearchResult::kError;
    }
  }();

  std::move(on_done_callback)
      .Run(model::SearchResponse{search_result, result.error,
                                 std::move(results)});
}

}  // namespace engine::game
