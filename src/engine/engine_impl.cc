#include "engine/engine_impl.h"

#include "nlohmann/json.hpp"

#include "engine/games/contractors/contractors_game.h"
#include "engine/games/pavlov/pavlov_game.h"

namespace engine {

namespace {
void SetGameStatusText(Presenter* presenter,
                       const std::string& game,
                       std::string status_text) {
  presenter->SetStatusText("[" + game + "] " + status_text);
}

void ReportGameMessage(Presenter* presenter,
                       Presenter::MessageType type,
                       std::string message,
                       std::string details) {
  presenter->ReportMessage(type, std::move(message), std::move(details));
}
}  // namespace

AppEngineImpl::AppEngineImpl(std::string steam_auth_backend_command,
                             Presenter* presenter)
    : steam_auth_backend_command_(std::move(steam_auth_backend_command)),
      presenter_(presenter) {
  config_.LoadConfig();
}

AppEngineImpl::~AppEngineImpl() {
  config_.SaveConfig();
}

std::vector<std::string> AppEngineImpl::GetSupportedGames() const {
  return {
      "Pavlov",
      "Contractors",
  };
}

std::vector<std::string> AppEngineImpl::GetEnabledGames() const {
  return config_.GetConfig().app.startup.enabled_games;
}

std::string AppEngineImpl::GetSelectedGame() const {
  return config_.GetConfig().app.startup.selected_game;
}

void AppEngineImpl::Initialize() {
  // Load enabled games
  SetEnabledGames(config_.GetConfig().app.startup.enabled_games);

  // ...
}

void AppEngineImpl::SetEnabledGames(std::vector<std::string> enabled_games) {
  auto& config = config_.GetConfig();

  if (!games_.empty() && enabled_games == config.app.startup.enabled_games) {
    return;
  }

  games_.clear();

  std::vector<std::string> loaded_game_names;
  std::vector<model::Game> loaded_game_models;
  for (const auto& enabled_game : enabled_games) {
    if (const auto* game = LoadGame(enabled_game, config)) {
      loaded_game_names.push_back(enabled_game);
      loaded_game_models.push_back(game->GetModel());
    } else {
      if (config.app.startup.selected_game == enabled_game) {
        config.app.startup.selected_game.clear();
      }
      presenter_->ReportMessage(Presenter::MessageType::kError,
                                "Failed to initialize game '" + enabled_game +
                                    "' - game won't be available",
                                "");
    }
  }
  config.app.startup.enabled_games = loaded_game_names;
  if (config.app.startup.selected_game.empty() && !loaded_game_names.empty()) {
    config.app.startup.selected_game = loaded_game_names.front();
  }

  nlohmann::json ui_config;
  if (auto cfg_it = config.ui.find(presenter_->GetPresenterName());
      cfg_it != config.ui.end()) {
    ui_config = cfg_it->second;
  }

  presenter_->Initialize(config.app, ui_config, std::move(loaded_game_models));
}

void AppEngineImpl::SetSearchOnStartup(bool enabled) {
  config_.GetConfig().app.startup.search_on_startup = enabled;
}

void AppEngineImpl::SetSelectedGame(std::string selected_game) {
  for (const auto& game : config_.GetConfig().app.startup.enabled_games) {
    if (game == selected_game) {
      config_.GetConfig().app.startup.selected_game = selected_game;
      return;
    }
  }

  presenter_->ReportMessage(
      Presenter::MessageType::kFatalError,
      "Failed to select game '" + selected_game + "' - game not found", "");
}

void AppEngineImpl::SearchLobbiesAndServers(
    model::SearchRequest request,
    base::OnceCallback<void(model::SearchResponse)> on_done_callback) {
  auto game_it = games_.find(request.game_name);
  if (game_it == games_.end()) {
    presenter_->ReportMessage(Presenter::MessageType::kError,
                              "Failed to search lobbies for '" +
                                  request.game_name + "' - game not found",
                              "");
    std::move(on_done_callback)
        .Run(model::SearchResponse{
            model::SearchResult::kError, "Game not found", {}});
    return;
  }

  auto& game = *game_it->second;
  game.SearchLobbiesAndServers(std::move(request), std::move(on_done_callback));
}

void AppEngineImpl::GetServerLobbyDetails(
    model::SearchDetailsRequest request,
    base::OnceCallback<void(model::SearchDetailsResponse)> on_done_callback) {
  auto game_it = games_.find(request.game_name);
  if (game_it == games_.end()) {
    presenter_->ReportMessage(Presenter::MessageType::kError,
                              "Failed to search lobby details for '" +
                                  request.game_name + "' - game not found",
                              "");
    std::move(on_done_callback).Run({});
    return;
  }

  auto& game = *game_it->second;
  game.GetServerLobbyDetails(std::move(request), std::move(on_done_callback));
}

void AppEngineImpl::UpdateGameConfig(std::string game_name,
                                     model::GameFilters filters) {
  auto game_it = games_.find(game_name);
  if (game_it == games_.end()) {
    presenter_->SetStatusText("Failed to update config for " + game_name +
                              " - game not found");
    return;
  }

  auto& game = *game_it->second;
  auto game_config = game.UpdateConfig(std::move(filters));
  if (!game_config.is_null()) {
    config_.GetConfig().games[game_name] = game_config;
  }
}

void AppEngineImpl::UpdateUiConfig(model::UiConfig config) {
  config_.GetConfig().ui[presenter_->GetPresenterName()] = config;
}

const game::Game* AppEngineImpl::LoadGame(const std::string& game,
                                          const model::Config& config) {
  if (game == "Pavlov") {
    nlohmann::json pavlov_config;
    if (auto cfg_it = config.games.find(game); cfg_it != config.games.end()) {
      pavlov_config = cfg_it->second;
    }

    games_[game] = std::make_unique<game::PavlovGame>(
        base::BindRepeating(&SetGameStatusText, presenter_, game),
        base::BindRepeating(&ReportGameMessage, presenter_), pavlov_config,
        steam_auth_backend_command_);
    return games_[game].get();
  }
  if (game == "Contractors") {
    nlohmann::json contractors_config;
    if (auto cfg_it = config.games.find(game); cfg_it != config.games.end()) {
      contractors_config = cfg_it->second;
    }

    games_[game] = std::make_unique<game::ContractorsGame>(
        base::BindRepeating(&SetGameStatusText, presenter_, game),
        base::BindRepeating(&ReportGameMessage, presenter_), contractors_config,
        steam_auth_backend_command_);
    return games_[game].get();
  }

  return nullptr;
}

}  // namespace engine
