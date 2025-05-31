#include "application.h"

#include "base/init.h"
#include "base/logging.h"
#include "base/net/init.h"

#include "engine/engine_impl.h"
#include "ui/wx/wx_presenter.h"
#include "utils/arg_parse.h"

bool Application::OnInit() {
  base::Initialize(argc, argv, {});
  base::net::Initialize({});

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

void Application::OnSearchOnStartupChanged(bool enabled) {
  engine_->SetSearchOnStartup(enabled);
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

void Application::UpdateGameConfig(std::string game_name,
                                   model::GameFilters filters) {
  engine_->UpdateGameConfig(std::move(game_name), std::move(filters));
}

void Application::UpdateUiConfig(model::UiConfig config) {
  engine_->UpdateUiConfig(std::move(config));
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
