#pragma once

#include "base/callback.h"
#include "nlohmann/json.hpp"

#include "models/game_config.h"
#include "models/search.h"

namespace engine::game {

class Game {
 public:
  virtual ~Game() = default;

  virtual model::Game GetModel() const = 0;
  virtual nlohmann::json UpdateConfig(model::GameFilters search_filters) = 0;
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
  virtual bool IsPlayerInFavorites(std::string player_id) const = 0;
  virtual void AddPlayerToFavorites(std::string player_id) = 0;
  virtual void RemovePlayerFromFavorites(std::string player_id) = 0;

  virtual model::GameConfigDescriptor GetConfigDescriptor() const = 0;
  virtual void UpdateConfigOption(std::string key, std::string value) = 0;
  virtual void AddConfigListItem(std::string key,
                                 std::vector<std::string> fields) = 0;
  virtual void RemoveConfigListItem(std::string key, std::string item_id) = 0;
  // Returns true if needs reinitialization
  virtual bool CommitConfig(model::GameConfigDescriptor descriptor) = 0;

  virtual void LaunchGame(std::string server_address) = 0;
};

}  // namespace engine::game
