#pragma once

#include <utility>

#include "base/memory/weak_ptr.h"
#include "base/net/resource_response.h"
#include "base/time/time_delta.h"
#include "nlohmann/json.hpp"

#include "engine/backends/eos/eos_data.h"
#include "engine/backends/result.h"
#include "engine/games/base_game.h"
#include "engine/games/pavlov/pavlov_data.h"
#include "engine/games/pavlov/pavlov_players_data_store.h"

namespace engine::backend {
class EosLobbyBackend;
}  // namespace engine::backend

namespace engine::game {

class PavlovGame : public BaseGame {
 public:
  PavlovGame(SetStatusTextCallback set_status_text,
             ReportMessageCallback report_message,
             nlohmann::json game_config,
             std::string steam_auth_backend_command);
  ~PavlovGame() override;

  // Game
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

 private:
  static pavlov::PavlovSearchResponse CombineSearchResponses(
      std::vector<pavlov::PavlovSearchResponse> responses);

  void UpdateGameVersionFromLobbyResults(
      backend::Result result,
      backend::eos::SearchLobbiesResponse response);
  void UpdateGameVersionFromLobbySession(
      const backend::eos::SearchLobbiesSession& session);
  void RequestServerList(
      pavlov::PavlovFilters request,
      base::OnceCallback<void(pavlov::PavlovSearchResponse)> on_done_callback);
  void OnSearchLobbiesDone(
      base::OnceCallback<void(pavlov::PavlovSearchResponse)> on_done_callback,
      backend::Result result,
      backend::eos::SearchLobbiesResponse response);
  void OnSearchServersDone(
      base::OnceCallback<void(pavlov::PavlovSearchResponse)> on_done_callback,
      base::net::ResourceResponse response);
  void OnServerPingReceived(
      base::OnceCallback<void(std::pair<std::string, base::TimeDelta>)>
          on_done_callback,
      std::string result_id,
      base::net::ResourceResponse response);
  void OnAllServersPingsReceived(
      base::OnceCallback<void(pavlov::PavlovSearchResponse)> on_done_callback,
      pavlov::PavlovSearchResponse partial_response,
      std::vector<std::pair<std::string, base::TimeDelta>>
          result_id_to_ping_vec);
  void StoreAndConvertSearchResults(
      base::OnceCallback<void(model::SearchResponse)> on_done_callback,
      pavlov::PavlovSearchResponse response);
  void OnFetchPlayersDataDone(backend::Result result,
                              backend::eos::FetchUsersInfoResponse response);
  void TryResolvePendingServerLobbyDetailsRequest(bool force);
  void OnSearchUsersDone(
      base::OnceCallback<void(model::SearchUsersResponse)> on_done_callback,
      backend::Result result,
      backend::eos::SearchUsersResponse response);
  void OnSearchUsersWithDetailsDone(
      base::OnceCallback<void(model::SearchUsersResponse)> on_done_callback,
      backend::eos::SearchUsersResponse response);

  bool IsPlayerInFavorties(const std::string& player_id) const;

  pavlov::PavlovConfig config_;
  std::unique_ptr<backend::EosLobbyBackend> lobby_backend_;
  pavlov::PavlovPlayersDataStore players_data_store_;
  std::optional<std::vector<pavlov::PavlovLobbyServer>> last_search_results_;
  std::optional<
      std::pair<model::SearchDetailsRequest,
                base::OnceCallback<void(model::SearchDetailsResponse)>>>
      pending_search_details_request_;

  base::WeakPtr<PavlovGame> weak_this_;
  base::WeakPtrFactory<PavlovGame> weak_factory_;
};

}  // namespace engine::game
