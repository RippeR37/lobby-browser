#pragma once

#include "base/callback.h"

#include "models/config.h"
#include "models/game_config.h"
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
  virtual void SetSelectedGame(std::string selected_game) = 0;
  virtual void SearchLobbiesAndServers(
      model::SearchRequest request,
      base::OnceCallback<void(model::SearchResponse)> on_done_callback) = 0;
  virtual void GetServerLobbyDetails(
      model::SearchDetailsRequest request,
      base::OnceCallback<void(model::SearchDetailsResponse)>
          on_done_callback) = 0;
  virtual void SearchUsers(model::SearchUsersRequest request,
                           base::OnceCallback<void(model::SearchUsersResponse)>
                               on_done_callback) = 0;
  virtual void UpdateGameConfig(std::string game_name,
                                model::GameFilters filters) = 0;
  virtual void UpdateUiConfig(model::UiConfig config) = 0;
  virtual bool IsPlayerInFavorites(std::string game_name,
                                   std::string player_id) const = 0;
  virtual void AddPlayerToFavorites(std::string game_name,
                                    std::string player_id) = 0;
  virtual void RemovePlayerFromFavorites(std::string game_name,
                                         std::string player_id) = 0;

  virtual model::GameConfigDescriptor GetGameConfigDescriptor(
      std::string game_name) const = 0;
  virtual void UpdateGameConfigOption(std::string game_name,
                                      std::string key,
                                      std::string value) = 0;
  virtual void AddGameConfigListItem(std::string game_name,
                                     std::string key,
                                     std::vector<std::string> fields) = 0;
  virtual void RemoveGameConfigListItem(std::string game_name,
                                        std::string key,
                                        std::string item_id) = 0;
  virtual void CommitGameConfig(std::string game_name,
                                model::GameConfigDescriptor descriptor) = 0;

  virtual void LaunchGame(std::string game_name,
                          std::string server_address) = 0;
};

}  // namespace engine
