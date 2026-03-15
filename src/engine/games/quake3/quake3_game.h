#pragma once

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "nlohmann/json.hpp"

#include "engine/backends/quake3/quake3_master_backend.h"
#include "engine/games/base_game.h"
#include "engine/games/quake3/quake3_data.h"
#include "models/game.h"
#include "models/search.h"

namespace engine::game {

class Quake3Game : public BaseGame {
 public:
  Quake3Game(SetStatusTextCallback set_status_text,
             ReportMessageCallback report_message,
             nlohmann::json game_config);
  ~Quake3Game() override;

  model::Game GetModel() const override;
  nlohmann::json UpdateConfig(model::GameFilters search_filters) override;
  void SearchLobbiesAndServers(model::SearchRequest request,
                               base::OnceCallback<void(model::SearchResponse)>
                                   on_done_callback) override;
  void GetServerLobbyDetails(
      model::SearchDetailsRequest request,
      base::OnceCallback<void(model::SearchDetailsResponse)> on_done_callback)
      override;
  void SearchUsers(model::SearchUsersRequest request,
                   base::OnceCallback<void(model::SearchUsersResponse)>
                       on_done_callback) override;

  model::GameConfigDescriptor GetConfigDescriptor() const override;
  void UpdateConfigOption(std::string key, std::string value) override;
  void AddConfigListItem(std::string key,
                         std::vector<std::string> fields) override;
  void RemoveConfigListItem(std::string key, std::string item_id) override;
  bool CommitConfig(model::GameConfigDescriptor descriptor) override;

  void LaunchGame(std::string server_address) override;

 private:
  struct SearchRequestState {
    size_t total_servers = 0;
    std::atomic<size_t> queried_servers{0};
  };

  void OnMasterSearchDone(
      base::OnceCallback<void(model::SearchResponse)> on_done_callback,
      base::OnceCallback<void(model::GamePlayersResults)> players_callback,
      backend::Quake3MasterSearchResponse response);
  void OnServerDetailsReceived(
      std::shared_ptr<SearchRequestState> state,
      base::RepeatingCallback<void(std::optional<model::GameServerLobbyResult>)>
          barrier_cb,
      std::string address,
      base::net::ResourceResponse response);
  void OnAllServersDetailsReceived(
      base::OnceCallback<void(model::SearchResponse)> on_done_callback,
      std::vector<std::optional<model::GameServerLobbyResult>> results);

  quake3::Quake3Config config_;
  std::unique_ptr<backend::Quake3MasterBackend> master_backend_;
  std::optional<std::vector<backend::Quake3ServerResult>>
      last_response_results_;

  base::WeakPtr<Quake3Game> weak_this_;
  base::WeakPtrFactory<Quake3Game> weak_factory_;
};

}  // namespace engine::game
