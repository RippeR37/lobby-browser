#pragma once

#include <string>
#include <vector>

#include "base/callback.h"

#include "models/config.h"
#include "models/search.h"

namespace ui {

class EventHandler {
 public:
  virtual ~EventHandler() = default;

  virtual std::vector<std::string> GetSupportedGames() const = 0;
  virtual void OnEnabledGamesChanged(
      std::vector<std::string> enabled_games) = 0;

  virtual void OnSearchOnStartupChanged(bool enabled) = 0;
  virtual void OnSelectedGameChanged(std::string selected_game) = 0;
  virtual void OnSearchLobbiesAndServers(
      model::SearchRequest request,
      base::OnceCallback<void(model::SearchResponse)> on_done_callback) = 0;
  virtual void OnServerLobbyDetailsRequested(
      model::SearchDetailsRequest request,
      base::OnceCallback<void(model::SearchDetailsResponse)>
          on_done_callback) = 0;

  virtual void UpdateGameConfig(std::string game_name,
                                model::GameFilters filters) = 0;
  virtual void UpdateUiConfig(model::UiConfig config) = 0;
};

}  // namespace ui
