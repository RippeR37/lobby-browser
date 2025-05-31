#pragma once

#include "base/callback.h"

#include "models/config.h"
#include "models/search.h"

namespace engine {

class AppEngine {
 public:
  virtual ~AppEngine() = default;

  virtual std::vector<std::string> GetSupportedGames() const = 0;
  virtual std::vector<std::string> GetEnabledGames() const = 0;
  virtual std::string GetSelectedGame() const = 0;

  virtual void Initialize() = 0;
  virtual void SetEnabledGames(std::vector<std::string> enabled_games) = 0;
  virtual void SetSearchOnStartup(bool enabled) = 0;
  virtual void SetSelectedGame(std::string selected_game) = 0;
  virtual void SearchLobbiesAndServers(
      model::SearchRequest request,
      base::OnceCallback<void(model::SearchResponse)> on_done_callback) = 0;
  virtual void GetServerLobbyDetails(
      model::SearchDetailsRequest request,
      base::OnceCallback<void(model::SearchDetailsResponse)>
          on_done_callback) = 0;
  virtual void UpdateGameConfig(std::string game_name,
                                model::GameFilters filters) = 0;
  virtual void UpdateUiConfig(model::UiConfig config) = 0;
};

}  // namespace engine
