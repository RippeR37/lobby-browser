#pragma once

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "nlohmann/json.hpp"

#include "engine/backends/modio/modio_mod_backend.h"
#include "engine/backends/nakama/nakama_data.h"
#include "engine/backends/nakama/nakama_lobby_backend.h"
#include "engine/games/base_game.h"
#include "engine/games/contractors/contractors_data.h"
#include "models/game.h"
#include "models/search.h"

namespace engine::game {

class ContractorsGame : public BaseGame {
 public:
  ContractorsGame(SetStatusTextCallback set_status_text,
                  ReportMessageCallback report_message,
                  nlohmann::json game_config,
                  std::string steam_auth_backend_command);
  ~ContractorsGame() override;

  model::Game GetModel() const override;
  nlohmann::json UpdateConfig(model::GameFilters search_filters) override;
  void SearchLobbiesAndServers(model::SearchRequest request,
                               base::OnceCallback<void(model::SearchResponse)>
                                   on_done_callback) override;
  void GetServerLobbyDetails(
      model::SearchDetailsRequest request,
      base::OnceCallback<void(model::SearchDetailsResponse)> on_done_callback)
      override;

 private:
  void OnModsFetched();
  void OnSearchLobbiesResponse(
      base::OnceCallback<void(model::SearchResponse)> on_done_callback,
      backend::Result result,
      backend::nakama::LobbySearchResponse response);

  contractors::ContractorsConfig config_;
  std::unique_ptr<backend::NakamaLobbyBackend> lobby_backend_;
  std::unique_ptr<backend::ModIoModBackend> mod_backend_;

  std::vector<base::OnceClosure> on_mods_fetched_callbacks_;

  base::WeakPtr<ContractorsGame> weak_this_;
  base::WeakPtrFactory<ContractorsGame> weak_factory_;
};

}  // namespace engine::game
