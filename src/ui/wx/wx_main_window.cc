#include "ui/wx/wx_main_window.h"

#include "wx/aboutdlg.h"
#include "wx/choicdlg.h"
#include "wx/imaglist.h"
#include "wx/menu.h"
#include "wx/notebook.h"
#include "wx/notifmsg.h"
#include "wx/panel.h"
#include "wx/rearrangectrl.h"
#include "wx/sizer.h"
#include "wx/toolbar.h"
#include "wx/toolbook.h"

#include "ui/wx/wx_auto_search_dialog.h"

namespace ui::wx {

namespace {
const auto kDefaultWindowTitle = "Lobby Browser";
const auto kDefaultWindowSize = wxSize(1200, 800);

enum MAIN_WINDOW_EVENT_IDS {
  ID_Menu_File_SearchOnStartup = 1,
  ID_Menu_File_AutoSearch = 2,

  ID_Menu_File_Settings_Games = 3,

  ID_Button_Search = 100,
};
}  // namespace

// static
std::pair<bool, std::vector<std::string>> WxMainWindow::AskForEnabledGames(
    wxWindow* parent,
    std::vector<std::string> supported_games,
    std::set<std::string> initial_selections) {
  wxArrayString game_choices;
  for (const auto& game : supported_games) {
    game_choices.Add(game);
  }

  wxArrayInt order;
  for (int idx = 0; static_cast<size_t>(idx) < game_choices.size(); ++idx) {
    if (initial_selections.find(supported_games[idx]) !=
        initial_selections.end()) {
      order.push_back(idx);
    } else {
      order.push_back(~idx);
    }
  }

  wxRearrangeDialog dlg{
      parent,
      "Select which games you want to enable in this application.",
      "Choose enabled games",
      order,
      game_choices,
  };
  dlg.Center();

  std::vector<std::string> enabled_games;
  if (dlg.ShowModal() == wxID_OK) {
    order = dlg.GetOrder();
    for (int selected_idx : order) {
      if (selected_idx >= 0) {
        enabled_games.emplace_back(supported_games[selected_idx]);
      }
    }
    return {true, std::move(enabled_games)};
  }

  return {false, {}};
}

WxMainWindow::WxMainWindow(
    EventHandler* event_handler,
    base::RepeatingCallback<void(Presenter::MessageType,
                                 std::string,
                                 std::string)> report_message_callback)
    : wxFrame(nullptr,
              wxID_ANY,
              kDefaultWindowTitle,
              wxDefaultPosition,
              kDefaultWindowSize,
              wxDEFAULT_FRAME_STYLE & ~(wxMAXIMIZE_BOX | wxRESIZE_BORDER)),
      event_handler_(event_handler),
      report_message_callback_(std::move(report_message_callback)),
      toolbook_(nullptr),
      weak_factory_(this) {
  weak_this_ = weak_factory_.GetWeakPtr();

  SetIcon(wxICON(IDI_ICON_MAIN));
  SetSizeHints(kDefaultWindowSize, kDefaultWindowSize);
  SetBackgroundColour(wxColour{0xFA, 0xFA, 0xFA});

  // Tray icon
  tray_icon_ = std::make_unique<WxTrayIcon>(this);
  tray_icon_->SetIcon(wxICON(IDI_ICON_MAIN), kDefaultWindowTitle);

  // Menu bar
  {
    auto* menu_bar = new wxMenuBar();

    auto* application_menu = new wxMenu();
    menu_startup_search_ = application_menu->AppendCheckItem(
        ID_Menu_File_SearchOnStartup, "Search on startup",
        "Performs an automatic server/lobby search on application startup");
    menu_auto_search_ = application_menu->AppendCheckItem(
        ID_Menu_File_AutoSearch, "Auto search",
        "Enables looped search for server/lobby with given criteria until any "
        "are found");
    application_menu->AppendSeparator();

    auto* settings = new wxMenu();
    application_menu->AppendSubMenu(settings, "Settings");
    settings->Append(ID_Menu_File_Settings_Games, "Games",
                     "Configure which games to enable");

    application_menu->AppendSeparator();
    application_menu->Append(wxID_EXIT);
    menu_bar->Append(application_menu, "&Application");

    auto* help_menu = new wxMenu();
    help_menu->Append(wxID_ABOUT);
    menu_bar->Append(help_menu, "&Help");

    SetMenuBar(menu_bar);

    Bind(
        wxEVT_MENU,
        [=](const wxCommandEvent&) {
          event_handler_->OnSearchOnStartupChanged(
              menu_startup_search_->IsChecked());
        },
        ID_Menu_File_SearchOnStartup);
    Bind(
        wxEVT_MENU, [=](const wxCommandEvent&) { OnAutoSearchChanged(); },
        ID_Menu_File_AutoSearch);
    Bind(
        wxEVT_MENU,
        [=](const wxCommandEvent&) {
          std::set<std::string> currently_enabled_games;
          for (const auto& [game, game_page] : game_pages_) {
            currently_enabled_games.insert(game);
          }

          // This could reinitialize app, so let's update config first
          UpdateConfig();

          auto enabled_games =
              AskForEnabledGames(this, event_handler_->GetSupportedGames(),
                                 currently_enabled_games);
          if (enabled_games.first) {
            event_handler_->OnEnabledGamesChanged(
                std::move(enabled_games.second));
          }
        },
        ID_Menu_File_Settings_Games);
  }

  // Status bar
  CreateStatusBar();
  CreateMainPanel();

  Bind(wxEVT_ICONIZE, &WxMainWindow::OnMinimize, this);
  Bind(wxEVT_CHAR_HOOK, &WxMainWindow::OnKeyDown, this);
  Bind(wxEVT_MENU, &WxMainWindow::OnAbout, this, wxID_ABOUT);
  Bind(wxEVT_MENU, &WxMainWindow::OnExit, this, wxID_EXIT);

  toolbook_->Bind(wxEVT_TOOLBOOK_PAGE_CHANGING, &WxMainWindow::OnGameChanging,
                  this);
  toolbook_->Bind(wxEVT_TOOLBOOK_PAGE_CHANGED, &WxMainWindow::OnGameChanged,
                  this);
  toolbook_->GetToolBar()->Bind(wxEVT_TOOL, &WxMainWindow::OnGameClicked, this);
  toolbook_->GetToolBar()->Bind(wxEVT_MOTION, &WxMainWindow::OnToolbarMouseMove,
                                this);
  toolbook_->GetToolBar()->Bind(wxEVT_LEAVE_WINDOW,
                                &WxMainWindow::OnToolbarMouseLeave, this);
}

WxMainWindow::~WxMainWindow() {
  UpdateConfig();
}

void WxMainWindow::CreatePageForGame(const model::Game& game, bool selected) {
  wxBitmap icon_bitmap(game.icon_path, wxBITMAP_TYPE_PNG_RESOURCE);
  wxImage icon_image = icon_bitmap.ConvertToImage();

  const auto size = toolbook_->GetImageList()->GetSize();
  wxImage scaledImage =
      icon_image.Scale(size.GetWidth(), size.GetHeight(), wxIMAGE_QUALITY_HIGH);
  wxBitmap scaledBitmap = scaledImage;

  auto idx = toolbook_->GetImageList()->Add(scaledBitmap);
  toolbook_->AddPage(CreateToolbookGamePage(toolbook_, game), game.name,
                     selected, idx);
}

void WxMainWindow::Initialize(const model::AppConfig& app_config,
                              const model::UiConfig& ui_config,
                              std::vector<model::Game> game_models) {
  if (!ui_config.is_null()) {
    try {
      config_ = ui_config;
    } catch (const std::exception& e) {
      LOG(ERROR) << __FUNCTION__
                 << "() failed to load game config: " << e.what();
    }
  } else {
    config_ = UiConfig{};
  }

  if (config_->startup.initial_position.x > 0 &&
      config_->startup.initial_position.y > 0) {
    SetPosition(wxPoint{config_->startup.initial_position.x,
                        config_->startup.initial_position.y});
  } else {
    CentreOnScreen();
  }

  if (game_models.empty()) {
    // This will re-initialize it
    auto enabled_games =
        AskForEnabledGames(this, event_handler_->GetSupportedGames(), {});
    while (enabled_games.second.empty()) {
      enabled_games =
          AskForEnabledGames(this, event_handler_->GetSupportedGames(), {});
    }
    event_handler_->OnEnabledGamesChanged(std::move(enabled_games.second));
    return;
  }

  toolbook_->DeleteAllPages();
  game_pages_.clear();

  for (const auto& game : game_models) {
    CreatePageForGame(game, game.name == app_config.startup.selected_game);
  }
  toolbook_->Realize();

  for (const auto& [game_name, game_cfg] : config_->games) {
    if (auto* game_page = GetGamePage(game_name)) {
      game_page->SetColumnSorting(WxGamePage::ColumnSortInfo{
          game_cfg.view.sort_by_column, game_cfg.view.order_ascending});
    }
  }

  if (app_config.startup.search_on_startup) {
    menu_startup_search_->Check();
    RefreshResultsForCurrentGame();
  }
}

void WxMainWindow::RefreshResultsForCurrentGame() {
  if (auto* game_page = GetCurrentGamePage()) {
    game_page->TriggerSearchIfPossible();
  }
}

void WxMainWindow::UpdateConfig() {
  // Update config of each game
  for (const auto& [game_name, game] : game_pages_) {
    event_handler_->UpdateGameConfig(game_name, game->GetCurrentGameFilters());

    const auto [sort_column, order_asc] = game->GetColumnSortInfo();
    config_->games[game_name].view = UiGameView{sort_column, order_asc};
  }

  // Update UI config
  if (!config_) {
    config_ = UiConfig{};
  }
  GetPosition(&config_->startup.initial_position.x,
              &config_->startup.initial_position.y);
  event_handler_->UpdateUiConfig(config_);
}

void WxMainWindow::OnGameChanging(wxBookCtrlEvent& event) {
  size_t new_game_idx = event.GetSelection();
  if (new_game_idx < 0 || new_game_idx >= game_pages_.size()) {
    report_message_callback_.Run(
        Presenter::MessageType::kError,
        "Invalid action performed, cannot switch to different game at this "
        "moment.\nPlease try again or restart application.",
        "");
    event.Skip();
    return;
  }

  event_handler_->OnSelectedGameChanged(GetGameNameByPageIdx(new_game_idx));

  // If need be to block change of the selected game, do `event.Veto()`
  event.Skip();
}

void WxMainWindow::OnGameChanged(wxBookCtrlEvent& event) {
  wxToolBarBase* toolbar = toolbook_->GetToolBar();

  // Remove the "pressed" (selected) state from all tools
  for (size_t i = 0; i < toolbook_->GetPageCount(); ++i) {
    toolbar->ToggleTool(toolbar->GetToolByPos(static_cast<int>(i))->GetId(),
                        false);
  }

  // Optionally, disable all other tools except the selected one
  int sel = toolbook_->GetSelection();
  for (size_t i = 0; i < toolbook_->GetPageCount(); ++i) {
    wxToolBarToolBase* tool = toolbar->GetToolByPos(static_cast<int>(i));
    toolbar->EnableTool(tool->GetId(), static_cast<int>(i) == sel);
  }

  toolbar->Refresh();
  event.Skip();
}

void WxMainWindow::OnGameClicked(wxCommandEvent& event) {
  int id = event.GetId();
  toolbook_->GetToolBar()->ToggleTool(id, false);
  toolbook_->GetToolBar()->Refresh();
  event.Skip();
}

void WxMainWindow::OnToolbarMouseMove(wxMouseEvent& event) {
  wxPoint pos = event.GetPosition();
  wxToolBarToolBase* tool =
      toolbook_->GetToolBar()->FindToolForPosition(pos.x, pos.y);

  if (tool) {
    // Find the index of the hovered tool
    for (size_t i = 0; i < toolbook_->GetPageCount(); ++i) {
      if (toolbook_->GetToolBar()->GetToolByPos(static_cast<int>(i)) == tool) {
        UpdateToolStates(-1, static_cast<int>(i));
        return;
      }
    }
  }

  // No valid tool under mouse
  UpdateToolStates();
  event.Skip();
}

void WxMainWindow::OnToolbarMouseLeave(wxMouseEvent& event) {
  UpdateToolStates();
  event.Skip();
}

void WxMainWindow::UpdateToolStates(int selected, int hover) {
  int sel = selected >= 0 ? selected : toolbook_->GetSelection();

  for (size_t i = 0; i < toolbook_->GetPageCount(); ++i) {
    wxToolBarToolBase* tool =
        toolbook_->GetToolBar()->GetToolByPos(static_cast<int>(i));
    if (static_cast<int>(i) == sel || static_cast<int>(i) == hover) {
      toolbook_->GetToolBar()->EnableTool(tool->GetId(), true);
    } else {
      toolbook_->GetToolBar()->EnableTool(tool->GetId(), false);
    }
  }
  toolbook_->GetToolBar()->Refresh();
}

void WxMainWindow::OnAutoSearchChanged() {
  if (menu_auto_search_->IsChecked()) {
    StartAutoSearch();
  } else {
    StopAutoSearch();
  }
}

void WxMainWindow::OnMinimize(wxIconizeEvent& event) {
  if (event.IsIconized()) {
    Hide();
  }
}

void WxMainWindow::OnAbout(wxCommandEvent&) {
  wxAboutDialogInfo info;
  info.SetName(_(kDefaultWindowTitle));
  info.SetVersion(_("1.0"));
  info.SetDescription(
      _("\nThis is a simple application allowing you to browse online\n"
        "game servers and lobbies without having to launch games."));
  info.SetCopyright(wxT("(C) 2025 RippeR37"));
  wxAboutBox(info);
}

void WxMainWindow::OnExit(wxCommandEvent&) {
  Close(true);
}

void WxMainWindow::OnKeyDown(wxKeyEvent& event) {
  switch (event.GetKeyCode()) {
    case WXK_F5:
      if (auto* current_game = GetCurrentGamePage()) {
        current_game->TriggerSearchIfPossible();
      }
      break;
  }
  event.Skip();
}

void WxMainWindow::CreateMainPanel() {
  auto* main_panel = new wxPanel(this);
  main_panel->SetBackgroundColour(wxColour{0xFA, 0xFA, 0xFA});

  // Setup wxToolbook
  toolbook_ = new wxToolbook(main_panel, wxID_ANY, wxDefaultPosition,
                             wxDefaultSize, wxNB_LEFT);
  toolbook_->AssignImageList(new wxImageList(48, 48, true));

  // Layout
  auto* mainSizer = new wxBoxSizer(wxVERTICAL);
  mainSizer->Add(toolbook_, 1, wxEXPAND | wxALL, 10);
  main_panel->SetSizer(mainSizer);

  // Color
  toolbook_->SetBackgroundColour(wxColour{0xFA, 0xFA, 0xFA});
  toolbook_->GetToolBar()->SetBackgroundColour(wxColour{0xFA, 0xFA, 0xFA});
}

wxPanel* WxMainWindow::CreateToolbookGamePage(wxWindow* parent,
                                              const model::Game& game) {
  game_pages_[game.name] = std::make_unique<WxGamePage>(
      parent, event_handler_, game,
      base::BindRepeating(&WxMainWindow::OnAutoSearchDone, weak_this_));
  auto* result = game_pages_[game.name]->GetMainPanel();
  return result;
}

WxGamePage* WxMainWindow::GetGamePage(const std::string& game_name) const {
  auto game_page_it = game_pages_.find(game_name);
  if (game_page_it != game_pages_.end()) {
    return game_page_it->second.get();
  }
  return nullptr;
}

WxGamePage* WxMainWindow::GetCurrentGamePage() const {
  auto* page = toolbook_->GetCurrentPage();
  for (const auto& game_page : game_pages_) {
    if (page == game_page.second->GetMainPanel()) {
      return game_page.second.get();
    }
  }
  return nullptr;
}

std::string WxMainWindow::GetGameNameByPageIdx(std::size_t page_idx) const {
  DCHECK_LT(page_idx, toolbook_->GetPageCount());

  auto* page = toolbook_->GetPage(page_idx);
  for (const auto& [game_name, game_page] : game_pages_) {
    if (page == game_page->GetMainPanel()) {
      return game_name;
    }
  }

  throw std::out_of_range{"Could not find game page with idx " +
                          std::to_string(page_idx)};
}

std::string WxMainWindow::GetCurrentGameName() const {
  auto* page = toolbook_->GetCurrentPage();
  for (const auto& [game_name, game_page] : game_pages_) {
    if (page == game_page->GetMainPanel()) {
      return game_name;
    }
  }

  throw std::out_of_range{"Could not find current game page"};
}

void WxMainWindow::StartAutoSearch() {
  auto* game_page = GetCurrentGamePage();
  if (!game_page) {
    LOG(WARNING) << "AutoSearch cannot be enabled: no game page found";
    return;
  }

  WxAutoSearchDialog dialog(this, game_page->GetGameModel().results_format);
  auto result = dialog.ShowModal();
  if (result != wxID_OK) {
    LOG(INFO) << "AutoSearch() canceled";
    menu_auto_search_->Check(false);
    return;
  }

  LOG(INFO) << "AutoSearch() auto search enabled";

  SetIcon(wxICON(IDI_ICON_SEARCHING));
  tray_icon_->SetIcon(wxICON(IDI_ICON_SEARCHING), kDefaultWindowTitle);

  game_page->StartAutoSearch(dialog.GetFilterOptions());
}

void WxMainWindow::StopAutoSearch() {
  SetIcon(wxICON(IDI_ICON_MAIN));
  tray_icon_->SetIcon(wxICON(IDI_ICON_MAIN), kDefaultWindowTitle);

  if (menu_auto_search_->IsChecked()) {
    menu_auto_search_->Check(false);
  }

  for (const auto& [game_name, game_page] : game_pages_) {
    game_page->StopAutoSearch();
  }
}

void WxMainWindow::OnAutoSearchDone(size_t matching_count) {
  StopAutoSearch();

  const auto kNotificationAutoHide = base::Seconds(20);

  wxNotificationMessage lobby_found_msg;
  lobby_found_msg.SetTitle(GetCurrentGameName() + " lobby search finished");
  lobby_found_msg.SetMessage("Found " + std::to_string(matching_count) +
                             " result(s) matching specified criteria.");
  lobby_found_msg.UseTaskBarIcon(tray_icon_.get());
  lobby_found_msg.Show(static_cast<int>(kNotificationAutoHide.InSeconds()));
}

}  // namespace ui::wx
