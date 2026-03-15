#include "application.h"

#include "base/init.h"
#include "base/logging.h"
#include "base/net/init.h"
#include "ixwebsocket/IXNetSystem.h"

#include "engine/engine_impl.h"
#include "ui/wx/wx_presenter.h"
#include "utils/arg_parse.h"

bool Application::OnInit() {
  base::Initialize(argc, argv, {});
  base::net::Initialize({});
  ix::initNetSystem();

  message_loop_attachment_ =
      std::make_unique<base::wx::WxMessageLoopAttachment>(this);

  try {
    const auto* kSteamAuthBackendCommand = "steam_auth_client";

    ui_ = std::make_unique<ui::wx::WxPresenter>(this);
    engine_ = std::make_unique<engine::AppEngineImpl>(kSteamAuthBackendCommand,
                                                      ui_.get());

    engine_->Initialize();
    ui_->ShowMainWindow();
  } catch (const std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << "() initialization failed: " << e.what();
    throw;
  }

  LOG(INFO) << "Application initialized";
  return true;
}

int Application::OnExit() {
  LOG(INFO) << "Application shutting down";

  engine_.reset();
  ui_.reset();

  message_loop_attachment_.reset();

  ix::uninitNetSystem();
  base::net::Deinitialize();
  base::Deinitialize();
  return 0;
}

std::vector<std::string> Application::GetSupportedGames() const {
  return engine_->GetSupportedGames();
}

void Application::OnEnabledGamesChanged(
    std::vector<std::string> enabled_games) {
  engine_->SetEnabledGames(std::move(enabled_games));
}

void Application::OnSelectedGameChanged(std::string selected_game) {
  engine_->SetSelectedGame(std::move(selected_game));
}

void Application::OnSearchLobbiesAndServers(
    model::SearchRequest request,
    base::OnceCallback<void(model::SearchResponse)> on_done_callback) {
  engine_->SearchLobbiesAndServers(std::move(request),
                                   std::move(on_done_callback));
}

void Application::OnServerLobbyDetailsRequested(
    model::SearchDetailsRequest request,
    base::OnceCallback<void(model::SearchDetailsResponse)> on_done_callback) {
  engine_->GetServerLobbyDetails(std::move(request),
                                 std::move(on_done_callback));
}

void Application::OnSearchUsers(
    model::SearchUsersRequest request,
    base::OnceCallback<void(model::SearchUsersResponse)> on_done_callback) {
  engine_->SearchUsers(std::move(request), std::move(on_done_callback));
}

void Application::UpdateGameConfig(std::string game_name,
                                   model::GameFilters filters) {
  engine_->UpdateGameConfig(std::move(game_name), std::move(filters));
}

void Application::UpdateUiConfig(model::UiConfig config) {
  engine_->UpdateUiConfig(std::move(config));
}

bool Application::IsPlayerInFavorites(std::string game_name,
                                      std::string player_id) const {
  return engine_->IsPlayerInFavorites(std::move(game_name),
                                      std::move(player_id));
}

void Application::AddPlayerToFavorites(std::string game_name,
                                       std::string player_id) {
  engine_->AddPlayerToFavorites(std::move(game_name), std::move(player_id));
}

void Application::RemovePlayerFromFavorites(std::string game_name,
                                            std::string player_id) {
  engine_->RemovePlayerFromFavorites(std::move(game_name),
                                     std::move(player_id));
}

model::GameConfigDescriptor Application::GetGameConfigDescriptor(
    std::string game_name) const {
  return engine_->GetGameConfigDescriptor(std::move(game_name));
}

void Application::UpdateGameConfigOption(std::string game_name,
                                         std::string key,
                                         std::string value) {
  engine_->UpdateGameConfigOption(std::move(game_name), std::move(key),
                                  std::move(value));
}

void Application::AddGameConfigListItem(std::string game_name,
                                        std::string key,
                                        std::vector<std::string> fields) {
  engine_->AddGameConfigListItem(std::move(game_name), std::move(key),
                                 std::move(fields));
}

void Application::RemoveGameConfigListItem(std::string game_name,
                                           std::string key,
                                           std::string item_id) {
  engine_->RemoveGameConfigListItem(std::move(game_name), std::move(key),
                                    std::move(item_id));
}

void Application::CommitGameConfig(std::string game_name,
                                   model::GameConfigDescriptor descriptor) {
  engine_->CommitGameConfig(std::move(game_name), std::move(descriptor));
}

void Application::OnLaunchGame(std::string game_name,
                               std::string server_address) {
  engine_->LaunchGame(std::move(game_name), std::move(server_address));
}

bool Application::OnExceptionInMainLoop() {
  try {
    throw;  // Rethrow the current exception.
  } catch (const std::exception& e) {
    LOG(ERROR) << "Uncaught exception: " << e.what();
  } catch (...) {
    LOG(ERROR) << "Uncaught unknown exception";
  }

  // Exit the main loop and thus terminate the program.
  return false;
}
