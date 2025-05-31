#pragma once

#include <memory>

#include "base/message_loop/wx/wx_message_loop_attachment.h"
#include "wx/app.h"

#include "engine/engine.h"
#include "ui/event_handler.h"
#include "ui/presenter.h"

class Application : public wxApp, public ui::EventHandler {
 public:
  // wxApp
  bool OnInit() override;
  int OnExit() override;

  // ui::EventHandler
  std::vector<std::string> GetSupportedGames() const override;
  void OnEnabledGamesChanged(std::vector<std::string> enabled_games) override;
  void OnSearchOnStartupChanged(bool enabled) override;
  void OnSelectedGameChanged(std::string selected_game) override;
  void OnSearchLobbiesAndServers(model::SearchRequest request,
                                 base::OnceCallback<void(model::SearchResponse)>
                                     on_done_callback) override;
  void OnServerLobbyDetailsRequested(
      model::SearchDetailsRequest request,
      base::OnceCallback<void(model::SearchDetailsResponse)> on_done_callback)
      override;
  void UpdateGameConfig(std::string game_name,
                        model::GameFilters filters) override;
  void UpdateUiConfig(model::UiConfig config) override;

 private:
  // wxApp
  bool OnExceptionInMainLoop() override;

  std::unique_ptr<base::wx::WxMessageLoopAttachment> message_loop_attachment_;
  std::unique_ptr<ui::Presenter> ui_;
  std::unique_ptr<engine::AppEngine> engine_;
};
