#pragma once

#include "base/callback.h"
#include "nlohmann/json.hpp"

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
};

}  // namespace engine::game
