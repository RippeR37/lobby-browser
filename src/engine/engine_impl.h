#pragma once

#include <map>
#include <memory>
#include <string>

#include "engine/config/config_loader.h"
#include "engine/engine.h"
#include "engine/games/game.h"
#include "engine/presenter.h"
#include "models/config.h"

namespace engine {

class AppEngineImpl : public AppEngine {
 public:
  AppEngineImpl(std::string steam_auth_backend_command, Presenter* presenter);
  ~AppEngineImpl() override;

  // AppEngine
  std::vector<std::string> GetSupportedGames() const override;
  std::vector<std::string> GetEnabledGames() const override;
  std::string GetSelectedGame() const override;
  void Initialize() override;
  void SetEnabledGames(std::vector<std::string> enabled_games) override;
  void SetSearchOnStartup(bool enabled) override;
  void SetSelectedGame(std::string selected_game) override;
  void SearchLobbiesAndServers(model::SearchRequest request,
                               base::OnceCallback<void(model::SearchResponse)>
                                   on_done_callback) override;
  void GetServerLobbyDetails(
      model::SearchDetailsRequest request,
      base::OnceCallback<void(model::SearchDetailsResponse)> on_done_callback)
      override;
  void UpdateGameConfig(std::string game_name,
                        model::GameFilters filters) override;
  void UpdateUiConfig(model::UiConfig config) override;

 private:
  const game::Game* LoadGame(const std::string& game,
                             const model::Config& config);

  std::string steam_auth_backend_command_;
  Presenter* presenter_;
  config::ConfigLoader config_;
  std::map<std::string, std::unique_ptr<game::Game>> games_;
};

}  // namespace engine
