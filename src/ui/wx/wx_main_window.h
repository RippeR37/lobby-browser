#pragma once

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "wx/frame.h"
#include "wx/panel.h"
#include "wx/toolbook.h"

#include "models/config.h"
#include "models/game.h"
#include "ui/event_handler.h"
#include "ui/presenter.h"
#include "ui/wx/wx_game_page.h"
#include "ui/wx/wx_tray_icon.h"
#include "ui/wx/wx_ui_config.h"

namespace ui::wx {

class WxMainWindow : public wxFrame {
 public:
  static std::pair<bool, std::vector<std::string>> AskForEnabledGames(
      wxWindow* parent,
      std::vector<std::string> supported_games,
      std::set<std::string> initial_selections);

  WxMainWindow(EventHandler* event_handler,
               base::RepeatingCallback<
                   void(Presenter::MessageType, std::string, std::string)>
                   report_message_callback);
  ~WxMainWindow() override;

  void CreatePageForGame(const model::Game& game, bool selected);
  void Initialize(const model::AppConfig& app_config,
                  const model::UiConfig& ui_config,
                  std::vector<model::Game> game_models);
  void RefreshResultsForCurrentGame();

 private:
  void UpdateConfig();
  void OnGameChanging(wxBookCtrlEvent& event);
  void OnGameChanged(wxBookCtrlEvent& event);
  void OnGameClicked(wxCommandEvent& event);
  void OnToolbarMouseMove(wxMouseEvent& event);
  void OnToolbarMouseLeave(wxMouseEvent& event);
  void UpdateToolStates(int selected = -1, int hover = -1);
  void OnAutoSearchChanged();
  void OnMinimize(wxIconizeEvent& event);
  void OnAbout(wxCommandEvent& event);
  void OnExit(wxCommandEvent& event);
  void OnKeyDown(wxKeyEvent& event);

  void CreateMainPanel();
  wxPanel* CreateToolbookGamePage(wxWindow* parent, const model::Game& game);
  WxGamePage* GetGamePage(const std::string& game_name) const;
  WxGamePage* GetCurrentGamePage() const;
  std::string GetGameNameByPageIdx(std::size_t page_idx) const;
  std::string GetCurrentGameName() const;

  void StartAutoSearch();
  void StopAutoSearch();
  void OnAutoSearchDone(size_t matching_count);

  EventHandler* event_handler_;
  base::RepeatingCallback<
      void(Presenter::MessageType, std::string, std::string)>
      report_message_callback_;
  std::unique_ptr<WxTrayIcon> tray_icon_;

  wxToolbook* toolbook_;
  std::optional<UiConfig> config_;

  wxMenuItem* menu_startup_search_;
  wxMenuItem* menu_auto_search_;

  std::map<std::string, std::unique_ptr<WxGamePage>> game_pages_;

  base::WeakPtr<WxMainWindow> weak_this_;
  base::WeakPtrFactory<WxMainWindow> weak_factory_;
};

}  // namespace ui::wx
