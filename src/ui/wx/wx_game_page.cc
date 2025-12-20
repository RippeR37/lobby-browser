#include "ui/wx/wx_game_page.h"

#include "base/logging.h"
#include "wx/bmpbuttn.h"
#include "wx/checkbox.h"
#include "wx/itemattr.h"
#include "wx/menu.h"
#include "wx/menuitem.h"
#include "wx/sizer.h"
#include "wx/statbox.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"
#include "wx/utils.h"

#include "ui/wx/wx_lobby_connect_dialog.h"
#include "ui/wx/wx_players_list_model.h"
#include "ui/wx/wx_results_list_model.h"
#include "ui/wx/wx_theme.h"
#include "ui/wx/wx_web_view_members_template.h"
#include "utils/strings.h"

namespace ui::wx {

namespace {

enum GAME_PAGE_EVENT_IDS {
  ID_Lobby_ContextMenu_ShowDetails = 1001,
  ID_Lobby_ContextMenu_Connect = 1002,
  ID_Lobby_ContextMenu_PIN = 1003,  // noop

  ID_Players_ContextMenu_AddFavorite = 1004,
  ID_Players_ContextMenu_RemoveFavorite = 1005,
};

enum GAME_PAGE_PAGE_INDEX {
  kMainPageIdxLobbiesServers = 0,
  kMainPageIdxPlayers = 1,
};

bool MatchFilterField(std::string value,
                      std::string filter,
                      model::GameResultsColumnOrdering ordering) {
  value = util::ToLower(value);
  filter = util::ToLower(filter);

  switch (ordering) {
    case model::GameResultsColumnOrdering::kString: {
      auto filter_values = util::SplitString(filter, '|');
      for (const auto& filter_value : filter_values) {
        if (value.find(filter_value) != std::string::npos) {
          return true;
        }
      }
      return false;
    }

    case model::GameResultsColumnOrdering::kNumber:
      return std::stoll(value) == std::stoll(filter);

    // Player count
    case model::GameResultsColumnOrdering::kNumberDivNumber: {
      int value1 = 0, value2 = 0;
      if (std::sscanf(value.c_str(), "%d/%d", &value1, &value2) == 2) {
        return value1 >= std::stoll(filter);
      }
      return false;
    }

    case model::GameResultsColumnOrdering::kPingOrString:
      return value == filter;
  }
}

std::string GetTabTitleWithCount(std::string base,
                                 std::optional<size_t> count,
                                 std::optional<size_t> extra_count) {
  if (count) {
    base += " (" + std::to_string(*count) + ")";
  }
  if (extra_count) {
    base += " (" + std::to_string(*extra_count) + ")";
  }
  return base;
}

std::string GetLobbiesServersTabTitle(
    std::optional<size_t> results_count = std::nullopt,
    std::optional<size_t> extra_count = std::nullopt) {
  return GetTabTitleWithCount("Lobbies and Servers", results_count,
                              extra_count);
}

std::string GetPlayersTabTitle(
    std::optional<size_t> results_count = std::nullopt,
    std::optional<size_t> extra_count = std::nullopt) {
  return GetTabTitleWithCount("Players", results_count, extra_count);
}

}  // namespace

WxGamePage::WxGamePage(
    wxWindow* parent,
    EventHandler* event_handler,
    model::Game game_model,
    base::RepeatingCallback<void(size_t)> on_autosearch_found,
    WxThemeColors theme_colors)
    : parent_(parent),
      event_handler_(event_handler),
      game_model_(std::move(game_model)),
      on_autosearch_found_(std::move(on_autosearch_found)),
      theme_colors_(std::move(theme_colors)),
      main_panel_(new wxPanel(parent)),
      results_list_(nullptr),
      players_list_(nullptr),
      game_details_notebook_(nullptr),
      filters_panel_(nullptr),
      details_panel_(nullptr),
      search_button_(nullptr),
      switch_to_details_tab_(false),
      weak_factory_(this) {
  weak_this_ = weak_factory_.GetWeakPtr();

  main_panel_notebook_ = new wxNotebook(main_panel_, wxID_ANY);
  main_panel_notebook_->SetBackgroundColour(theme_colors_.GamePanelBg);

  // Add pages to the inner notebook
  main_panel_notebook_->AddPage(CreateMainGamePage(main_panel_notebook_),
                                GetLobbiesServersTabTitle());
  if (!game_model_.results_format.players_columns.empty()) {
    main_panel_notebook_->AddPage(CreatePlayersGamePage(main_panel_notebook_),
                                  GetPlayersTabTitle());
  }

  main_panel_notebook_->Bind(wxEVT_LEFT_DCLICK, [&](wxMouseEvent& event) {
    long flags = 0;
    int tab_index = main_panel_notebook_->HitTest(event.GetPosition(), &flags);
    if (tab_index != wxNOT_FOUND && (flags & wxNB_HITTEST_ONLABEL)) {
      // Double-clicked on tab `tab_index`
      TriggerSearchIfPossible();
    }
    event.Skip();
  });

  // Layout for the toolbook page
  auto* panelSizer = new wxBoxSizer(wxVERTICAL);
  panelSizer->Add(main_panel_notebook_, 1, wxEXPAND);
  main_panel_->SetSizer(panelSizer);
}

WxGamePage::~WxGamePage() = default;

wxPanel* WxGamePage::GetMainPanel() const {
  return main_panel_;
}

model::Game WxGamePage::GetGameModel() const {
  return game_model_;
}

model::GameFilters WxGamePage::GetCurrentGameFilters() const {
  model::GameFilters filters;

  auto* filters_panel_sizer = filters_panel_->GetSizer();
  if (!filters_panel_sizer) {
    return filters;
  }

  for (const auto* box_item : filters_panel_sizer->GetChildren()) {
    auto* box_sizer = dynamic_cast<wxStaticBoxSizer*>(box_item->GetSizer());
    if (!box_sizer) {
      continue;
    }

    // Get the group name
    model::GameFilterGroup filter_group;
    filter_group.name = box_sizer->GetStaticBox()->GetLabel();

    // The grid sizer with checkboxes is the first item
    for (const auto* box_child : box_sizer->GetChildren()) {
      auto* grid_sizer = dynamic_cast<wxGridSizer*>(box_child->GetSizer());
      if (!grid_sizer) {
        continue;
      }

      for (auto* grid_item : grid_sizer->GetChildren()) {
        if (!grid_item->IsWindow()) {
          continue;
        }

        auto* checkbox = dynamic_cast<wxCheckBox*>(grid_item->GetWindow());
        if (!checkbox) {
          continue;
        }

        filter_group.filter_options.push_back(model::GameFilterOption{
            checkbox->GetLabel().ToStdString(),
            checkbox->IsChecked(),
        });
      }
    }

    filters.push_back(filter_group);
  }

  return filters;
}

WxGamePage::ColumnSortInfo WxGamePage::GetColumnSortInfo() const {
  WxGamePage::ColumnSortInfo result{};
  if (auto* sort_column = results_list_->GetSortingColumn()) {
    result.column = results_list_->GetColumnIndex(sort_column);
    result.ascending = sort_column->IsSortOrderAscending();
  }
  return result;
}

void WxGamePage::SetColumnSorting(WxGamePage::ColumnSortInfo column_sorting) {
  if (column_sorting.column >= 0 &&
      column_sorting.column <
          static_cast<int>(results_list_->GetColumnCount())) {
    if (wxDataViewColumn* column =
            results_list_->GetColumn(column_sorting.column)) {
      column->SetSortOrder(column_sorting.ascending);
    }
  }
}

void WxGamePage::TriggerSearchIfPossible() {
  if (!search_button_->IsEnabled()) {
    return;
  };

  search_button_->Disable();

  results_list_->SetBackgroundColour(theme_colors_.ListLoadingBg);
  results_list_->Refresh();
  if (players_list_) {
    players_list_->SetBackgroundColour(theme_colors_.ListLoadingBg);
    players_list_->Refresh();
  }

  auto current_filters = GetCurrentGameFilters();
  event_handler_->OnSearchLobbiesAndServers(
      model::SearchRequest{
          game_model_.name,
          std::move(current_filters),
          base::BindOnce(&WxGamePage::OnSearchLobbiesPlayersDone, weak_this_),
      },
      base::BindOnce(&WxGamePage::OnSearchLobbiesAndServersDone, weak_this_));
}

void WxGamePage::ConnectToCurrentlySelectedLobbyIfPossible() {
  if (!game_model_.features.connect_to_lobby) {
    return;
  }

  auto selected_lobby = results_list_->GetCurrentItem();
  if (selected_lobby.IsOk()) {
    BringConnectToLobbyDialog(selected_lobby);
  }
}

void WxGamePage::StartAutoSearch(
    std::map<std::string, std::string> autosearch_options) {
  autosearch_options_ = std::move(autosearch_options);

  autosearch_timer_.Start(
      base::BindRepeating(&WxGamePage::TriggerSearchIfPossible, weak_this_),
      base::Seconds(30));
}

void WxGamePage::StopAutoSearch() {
  autosearch_options_.reset();
  autosearch_timer_.Stop();
}

void WxGamePage::OnRestoreFromTray() {
  if (game_model_.features.has_lobby_details) {
    const auto selected_page = game_details_notebook_->GetSelection();

    game_details_notebook_->RemovePage(1);
    game_details_notebook_->InsertPage(
        1, CreateMainGamePageDetailsDetailsPanel(game_details_notebook_),
        "Details");

    if (selected_result_id_ && last_search_details_) {
      OnServerLobbyDetailsReceived(*selected_result_id_, *last_search_details_);
    }

    if (selected_page != wxNOT_FOUND) {
      game_details_notebook_->SetSelection(selected_page);
    }
  }
}

wxPanel* WxGamePage::CreateMainGamePage(wxWindow* parent) {
  // First page of the notebook
  auto* main_game_page = new wxPanel(parent);
  main_game_page->SetBackgroundColour(theme_colors_.GamePageBg);

  // Layout for first notebook page
  auto* main_game_page_sizer = new wxBoxSizer(wxHORIZONTAL);
  main_game_page_sizer->Add(CreateMainGamePageLobbyListCtrl(
                                main_game_page, game_model_.results_format),
                            5, wxEXPAND | wxALL, 10);
  main_game_page_sizer->Add(CreateMainGamePageDetailsPanel(main_game_page), 2,
                            wxEXPAND | wxALL, 10);
  main_game_page->SetSizer(main_game_page_sizer);

  return main_game_page;
}

wxDataViewCtrl* WxGamePage::CreateMainGamePageLobbyListCtrl(
    wxWindow* parent,
    const model::GameResultsFormat& game_results_format) {
  auto* model = new WxResultsListModel(game_results_format.columns);
  results_list_ = new wxDataViewListCtrl(parent, wxID_ANY);
  results_list_->AssociateModel(model);
  model->DecRef();

  // Make headers bold
  wxFont headerFont = results_list_->GetFont();
  headerFont.MakeBold();
  wxItemAttr headerAttr{};
  headerAttr.SetFont(headerFont);
  results_list_->SetHeaderAttr(headerAttr);

  for (const auto& column : game_results_format.columns) {
    int flags = wxDATAVIEW_COL_SORTABLE;
    if (!column.visible) {
      flags |= wxDATAVIEW_COL_HIDDEN;
    }

    auto alignment = [&column]() {
      switch (column.alignment) {
        case model::GameResultsColumnAlignment::kLeft:
          return wxALIGN_LEFT;
        case model::GameResultsColumnAlignment::kCenter:
          return wxALIGN_CENTER;
        case model::GameResultsColumnAlignment::kRight:
          return wxALIGN_RIGHT;
      }
    }();

    results_list_->AppendTextColumn(column.name, wxDATAVIEW_CELL_INERT,
                                    static_cast<int>(column.proportion),
                                    alignment, flags);
  }

  results_list_->Bind(
      wxEVT_DATAVIEW_ITEM_ACTIVATED,
      [=, this](wxDataViewEvent& event) { OnRowEntered(event); });
  results_list_->Bind(
      wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, [=, this](wxDataViewEvent& event) {
        wxDataViewItem item = event.GetItem();
        if (!item.IsOk())
          return;

        if (!game_model_.features.has_lobby_details &&
            !game_model_.features.connect_to_lobby) {
          return;
        }

        int row = results_list_->ItemToRow(item);
        wxVariant value;
        results_list_->GetValue(value, row, 0);
        wxString lobby_id = value.GetString();

        wxMenu menu;

        if (game_model_.features.has_lobby_details) {
          menu.Append(ID_Lobby_ContextMenu_ShowDetails, "Show lobby details");
        }
        if (game_model_.features.connect_to_lobby) {
          // TODO: hack, filters out Palov servers
          if (lobby_id.find(":") == std::string::npos) {
            menu.Append(ID_Lobby_ContextMenu_Connect, "Connect to lobby");
          }
        }
        if (auto pin = GetLobbyMetadata(lobby_id.ToStdString(), "pin");
            pin && !(*pin).empty() && (*pin) != "EMPTY") {
          menu.AppendSeparator();

          auto* item = menu.Append(ID_Lobby_ContextMenu_PIN, "PIN: " + *pin,
                                   "PIN for locked lobby");
          item->Enable(false);
        }

        menu.Bind(wxEVT_MENU, [=, this](wxCommandEvent& evt) {
          OnRowContextMenuSelected(item, evt.GetId());
        });

        parent_->PopupMenu(&menu);
      });

  return results_list_;
}

wxPanel* WxGamePage::CreateMainGamePageDetailsPanel(wxWindow* parent) {
  auto* details_panel = new wxPanel(parent);
  auto* details_panel_sizer = new wxBoxSizer(wxVERTICAL);
  details_panel->SetSizer(details_panel_sizer);

  // Notebook
  game_details_notebook_ = new wxNotebook(details_panel, wxID_ANY);
  game_details_notebook_->SetBackgroundColour(theme_colors_.GameDetailsBg);
  details_panel_sizer->Add(game_details_notebook_, 20, wxEXPAND);

  // IMPORTANT: Update `OnRestoreFromTray()` if the order changes
  game_details_notebook_->AddPage(
      CreateMainGamePageDetailsFilterPanel(game_details_notebook_), "Filters");
  if (game_model_.features.has_lobby_details) {
    game_details_notebook_->AddPage(
        CreateMainGamePageDetailsDetailsPanel(game_details_notebook_),
        "Details");
  }

  // Action buttons
  search_button_ = new wxButton(details_panel, wxID_ANY, "Find Lobbies");
  details_panel_sizer->Add(search_button_, 0, wxALIGN_CENTER | wxTOP, 10);

  // Bind callback
  search_button_->Bind(
      wxEVT_BUTTON, [=, this](wxCommandEvent&) { TriggerSearchIfPossible(); });

  return details_panel;
}

wxPanel* WxGamePage::CreateMainGamePageDetailsFilterPanel(wxWindow* parent) {
  filters_panel_ = new wxPanel(parent);
  auto* filters_panel_sizer = new wxBoxSizer(wxVERTICAL);
  filters_panel_->SetSizer(filters_panel_sizer);

  for (const auto& group : game_model_.filter_groups) {
    const int rows = static_cast<int>(group.filter_options.size() + 1) / 2;
    const int columns = 2;
    const int proportion = rows + 1;  // +1 for header

    auto* box = new wxStaticBoxSizer(wxVERTICAL, filters_panel_, group.name);
    auto* box_grid = new wxGridSizer(rows, columns, 5, 5);
    box->Add(box_grid, 1, wxEXPAND | wxALL, 10);
    filters_panel_sizer->Add(box, proportion, wxEXPAND | wxALL, 5);

    for (const auto& option : group.filter_options) {
      auto* checkbox =
          new wxCheckBox(box->GetStaticBox(), wxID_ANY, option.name);

      checkbox->SetValue(option.enabled);
      if (option.refresh_on_change) {
        checkbox->Bind(wxEVT_CHECKBOX, [=, this](const wxCommandEvent&) {
          RefreshResultsList();
        });
      }

      box_grid->Add(checkbox, 0, wxALL, 5);
    }
  }

  return filters_panel_;
}

wxPanel* WxGamePage::CreateMainGamePageDetailsDetailsPanel(wxWindow* parent) {
  auto* panel = new wxPanel(parent);
  auto* panel_sizer = new wxBoxSizer(wxVERTICAL);
  panel->SetSizer(panel_sizer);

  if (details_panel_) {
    details_panel_->Destroy();
  }

  auto* details_panel = wxWebView::New(panel, wxID_ANY, wxWebViewDefaultURLStr,
                                       wxDefaultPosition, {});
  details_panel_ = details_panel;
  details_panel_->EnableContextMenu(false);
  details_panel_->EnableAccessToDevTools(false);

  // Install message handler with the name wx_msg
  details_panel_->AddScriptMessageHandler("wxWebView");
  details_panel_->Bind(
      wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, [=, this](wxWebViewEvent& evt) {
        if (evt.GetMessageHandler() == "wxWebView") {
          if (evt.GetString() == "keydown_F5") {
            TriggerSearchIfPossible();
          } else if (evt.GetString().starts_with("defaultBrowserNavigate;")) {
            const auto navigateUrl =
                evt.GetString().substr(sizeof("defaultBrowserNavigate;") - 1);
            if (!navigateUrl.empty()) {
              LOG(INFO) << "Navigate to: " << navigateUrl;
              wxLaunchDefaultBrowser(navigateUrl);
            }
          } else if (evt.GetString().starts_with("uiNavigate;")) {
            // navigate somewhere in UI
            if (evt.GetString().starts_with("uiNavigate;player;")) {
              // jump to player's view
              const auto player_id =
                  evt.GetString().substr(sizeof("uiNavigate;player;") - 1);
              if (player_id.size() > 0) {
                NavigateToPlayer(player_id.ToStdString());
              }
            }
          }
        }
      });

  details_panel_->Bind(wxEVT_WEBVIEW_LOADED, [details_panel](wxWebViewEvent&) {
    wxString script = R"(
      document.addEventListener('keydown', function(e) {
          if (e.key === 'F5') {
              window.wxWebView.postMessage('keydown_F5');
              e.preventDefault(); // prevent default page reload
          }
      });
    )";
    details_panel->RunScriptAsync(script);
  });

  panel_sizer->Add(details_panel_, 1, wxEXPAND | wxALL, 5);
  return panel;
}

wxPanel* WxGamePage::CreatePlayersGamePage(wxWindow* parent) {
  // Second page of the notebook
  auto* game_players_page = new wxPanel(parent);
  game_players_page->SetBackgroundColour(theme_colors_.GamePlayersPageBg);

  // Layout for second notebook page
  auto* game_players_page_sizer = new wxBoxSizer(wxHORIZONTAL);
  game_players_page_sizer->Add(
      CreateGamePagePlayersListCtrl(game_players_page,
                                    game_model_.results_format),
      5, wxEXPAND | wxALL, 10);
  game_players_page->SetSizer(game_players_page_sizer);

  return game_players_page;
}

wxDataViewCtrl* WxGamePage::CreateGamePagePlayersListCtrl(
    wxWindow* parent,
    const model::GameResultsFormat& game_results_format) {
  const int favorites_column = 2;

  auto* model = new WxPlayersListModel(favorites_column);
  players_list_ = new wxDataViewListCtrl(parent, wxID_ANY);
  players_list_->AssociateModel(model);
  model->DecRef();

  // Make headers bold
  wxFont headerFont = players_list_->GetFont();
  headerFont.MakeBold();
  wxItemAttr headerAttr{};
  headerAttr.SetFont(headerFont);
  players_list_->SetHeaderAttr(headerAttr);

  for (const auto& column : game_results_format.players_columns) {
    int flags = wxDATAVIEW_COL_SORTABLE;
    if (!column.visible) {
      flags |= wxDATAVIEW_COL_HIDDEN;
    }

    auto alignment = [&column]() {
      switch (column.alignment) {
        case model::GameResultsColumnAlignment::kLeft:
          return wxALIGN_LEFT;
        case model::GameResultsColumnAlignment::kCenter:
          return wxALIGN_CENTER;
        case model::GameResultsColumnAlignment::kRight:
          return wxALIGN_RIGHT;
      }
    }();

    auto* col = players_list_->AppendTextColumn(
        column.name, wxDATAVIEW_CELL_INERT, static_cast<int>(column.proportion),
        alignment, flags);

    if (column.name == "Player Name") {
      col->SetSortOrder(true);
    }
  }

  players_list_->Bind(
      wxEVT_DATAVIEW_ITEM_ACTIVATED,
      [=, this](wxDataViewEvent& event) { OnPlayersRowEntered(event); });
  players_list_->Bind(
      wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, [=, this](wxDataViewEvent& event) {
        wxDataViewItem item = event.GetItem();
        if (!item.IsOk())
          return;

        if (!game_model_.features.list_players) {
          return;
        }

        int row = players_list_->ItemToRow(item);
        wxVariant value;
        players_list_->GetValue(value, row, 1);
        wxString player_id = value.GetString();
        players_list_->GetValue(value, row, 4);
        wxString player_name = value.GetString();

        wxMenu menu;

        const bool is_in_favorites = event_handler_->IsPlayerInFavorites(
            game_model_.name, player_id.ToStdString());
        if (is_in_favorites) {
          menu.Append(ID_Players_ContextMenu_RemoveFavorite,
                      "Remove " + player_name + " from favorite");
        } else {
          menu.Append(ID_Players_ContextMenu_AddFavorite,
                      "Add " + player_name + " to favorites");
        }

        menu.Bind(wxEVT_MENU, [=, this](wxCommandEvent& evt) {
          OnPlayersRowContextMenuSelected(item, evt.GetId());
        });

        parent_->PopupMenu(&menu);
      });

  return players_list_;
}

void WxGamePage::OnRowEntered(wxDataViewEvent& event) {
  if (!game_model_.features.has_lobby_details) {
    return;
  }

  auto selected_row_item = event.GetItem();
  if (!selected_row_item.IsOk()) {
    return;
  }

  // Assume first is always ID, might need to be tweaked later?
  wxVariant id_variant;
  results_list_->GetStore()->GetValue(id_variant, selected_row_item, 0);

  ShowLobbyDetails(id_variant.GetString());
}

void WxGamePage::OnRowContextMenuSelected(wxDataViewItem selected_lobby,
                                          int event_id) {
  switch (event_id) {
    case ID_Lobby_ContextMenu_ShowDetails: {
      wxVariant id_variant;
      results_list_->GetStore()->GetValue(id_variant, selected_lobby, 0);
      ShowLobbyDetails(id_variant.GetString());
      break;
    }

    case ID_Lobby_ContextMenu_Connect: {
      BringConnectToLobbyDialog(selected_lobby);
      break;
    }
  }
}

void WxGamePage::OnPlayersRowContextMenuSelected(wxDataViewItem selected_lobby,
                                                 int event_id) {
  switch (event_id) {
    case ID_Players_ContextMenu_AddFavorite: {
      wxVariant player_id;
      players_list_->GetStore()->GetValue(player_id, selected_lobby, 1);
      event_handler_->AddPlayerToFavorites(game_model_.name,
                                           player_id.GetString().ToStdString());
      break;
    }

    case ID_Players_ContextMenu_RemoveFavorite: {
      wxVariant player_id;
      players_list_->GetStore()->GetValue(player_id, selected_lobby, 1);
      event_handler_->RemovePlayerFromFavorites(
          game_model_.name, player_id.GetString().ToStdString());
      break;
    }
  }
}

void WxGamePage::NavigateToPlayer(std::string player_id) {
  if (!game_model_.features.list_players) {
    return;
  }

  for (int row = 0; row < players_list_->GetItemCount(); ++row) {
    wxVariant row_variant;
    players_list_->GetValue(row_variant, row, 1);

    if (row_variant.GetString().IsSameAs(player_id)) {
      if (main_panel_notebook_->GetSelection() != 1) {
        main_panel_notebook_->SetSelection(1);
      }
      wxDataViewItem item = players_list_->RowToItem(row);
      players_list_->EnsureVisible(item);
      players_list_->Select(item);
      return;
    }
  }
}

void WxGamePage::ShowLobbyDetails(wxString lobby_id) {
  selected_result_id_ = lobby_id;
  switch_to_details_tab_ = true;

  // Don't want for full details as we want to present something as quickly as
  // possible. We will re-request full details if they aren't known yet when
  // we'll get partial ones.
  RequestSelectedLobbyDetails(false);
};

void WxGamePage::OnPlayersRowEntered(wxDataViewEvent& event) {
  auto selected_row_item = event.GetItem();
  if (!selected_row_item.IsOk()) {
    return;
  }

  // Assume first is always ID, might need to be tweaked later?
  wxVariant lobby_id_variant;
  players_list_->GetStore()->GetValue(lobby_id_variant, selected_row_item, 0);

  const auto lobby_id = lobby_id_variant.GetString();

  for (int row = 0; row < results_list_->GetItemCount(); ++row) {
    wxVariant row_variant;
    results_list_->GetValue(row_variant, row, 0);

    if (row_variant.GetString().IsSameAs(lobby_id)) {
      if (main_panel_notebook_->GetSelection() != 0) {
        main_panel_notebook_->SetSelection(0);
      }
      wxDataViewItem item = results_list_->RowToItem(row);
      results_list_->EnsureVisible(item);
      results_list_->Select(item);
      ShowLobbyDetails(row_variant.GetString());
      break;
    }
  }
}

void WxGamePage::OnSearchLobbiesAndServersDone(model::SearchResponse response) {
  search_button_->Enable();
  results_list_->SetBackgroundColour(theme_colors_.ListLoadedBg);
  results_list_->Refresh();

  if (response.result != model::SearchResult::kOk) {
    LOG(ERROR) << __FUNCTION__
               << "() result: " << static_cast<int>(response.result) << " - "
               << response.error;
    return;
  }

  last_response_results_ = std::move(response.results);
  create_lobby_connector_ = std::move(response.create_lobby_connector);
  RefreshResultsList();

  if (selected_result_id_) {
    // List of known members might have changed, so let's re-request details.
    RequestSelectedLobbyDetails(false);
  }
}

void WxGamePage::OnSearchLobbiesPlayersDone(model::GamePlayersResults players) {
  players_list_->SetBackgroundColour(theme_colors_.ListLoadedBg);
  players_list_->DeleteAllItems();
  players_list_->Refresh();

  const auto is_favorite = [](const model::GamePlayersResult& player_results) {
    // Hack, depends on a specific format, I guess.
    if (player_results.result_fields.size() < 3) {
      return false;
    }
    return !player_results.result_fields[2].empty();
  };

  const auto favorites_count =
      std::count_if(players.begin(), players.end(), is_favorite);

  main_panel_notebook_->SetPageText(
      kMainPageIdxPlayers, GetPlayersTabTitle(players.size(), favorites_count));

  // Append new players
  for (const auto& result : players) {
    std::vector<wxString> item_values;
    for (const auto& field : result.result_fields) {
      item_values.emplace_back(wxString::FromUTF8(field));
    };

    players_list_->AppendItem(
        wxVector<wxVariant>{item_values.begin(), item_values.end()});
  }
}

void WxGamePage::RequestSelectedLobbyDetails(bool wait_for_full_details) {
  CHECK(selected_result_id_);

  event_handler_->OnServerLobbyDetailsRequested(
      model::SearchDetailsRequest{game_model_.name, *selected_result_id_,
                                  wait_for_full_details},
      base::BindOnce(&WxGamePage::OnServerLobbyDetailsReceived, weak_this_,
                     *selected_result_id_));
}

void WxGamePage::OnServerLobbyDetailsReceived(
    std::string result_id,
    model::SearchDetailsResponse details) {
  if (result_id != selected_result_id_) {
    // Ignore
    return;
  }

  last_search_details_ = details;

  if (game_details_notebook_->GetPageCount() >= 2 && switch_to_details_tab_) {
    game_details_notebook_->SetSelection(1);
    switch_to_details_tab_ = false;
  }

  auto html_content = WxWebViewMembersHeader(theme_colors_.effective_theme);
  for (const auto& member : details.members) {
    html_content += WxWebViewMembersMemberRow(member);
  }
  html_content += WxWebViewMembersFooter();
  details_panel_->SetPage(html_content, "");

  if (!details.all_members_known) {
    // Engine didn't have all the members known at the time, so let's query
    // again but this time ask it to wait until they became known.
    RequestSelectedLobbyDetails(true);
  }

  results_list_->SetFocus();
}

void WxGamePage::RefreshResultsList() {
  results_list_->DeleteAllItems();
  lobbies_metadata_.clear();

  const auto filtered_results = game_model_.results_filter_callback.Run(
      last_response_results_, GetCurrentGameFilters());

  main_panel_notebook_->SetPageText(
      kMainPageIdxLobbiesServers,
      GetLobbiesServersTabTitle(filtered_results.lobbies.size()));

  // Append new results
  for (const auto& result : filtered_results.lobbies) {
    std::vector<wxString> item_values;
    for (const auto& field : result.result_fields) {
      item_values.emplace_back(wxString::FromUTF8(field));
    };

    if (!result.result_fields.empty()) {
      const auto lobby_id = result.result_fields.front();
      lobbies_metadata_[lobby_id] = result.metadata;
    }

    results_list_->AppendItem(
        wxVector<wxVariant>{item_values.begin(), item_values.end()});
  }

  // Select previously selected one, if still exists, otherwise clear memory
  if (!ReselectLastSelectedRow()) {
    // No longer exists, let's forget about it and
    selected_result_id_.reset();
    if (game_details_notebook_->GetPageCount() > 0) {
      game_details_notebook_->SetSelection(0);
    }
  }

  // Check autosearch options
  if (autosearch_options_) {
    if (size_t count = ResultsMatchingAutoSearchOptions(filtered_results);
        count > 0) {
      on_autosearch_found_.Run(count);
    }
  }
}

size_t WxGamePage::ResultsMatchingAutoSearchOptions(
    const model::GameSearchResults& results) const {
  if (!autosearch_options_) {
    return 0;
  }

  size_t matching_count = 0;

  auto match_result = [&](const model::GameServerLobbyResult& result) {
    for (size_t field_idx = 0; field_idx < result.result_fields.size();
         field_idx++) {
      const auto column_name =
          game_model_.results_format.columns[field_idx].name;
      auto autosearch_filter_it = autosearch_options_->find(column_name);
      if (autosearch_filter_it == autosearch_options_->end()) {
        continue;
      }

      if (!MatchFilterField(
              result.result_fields[field_idx], autosearch_filter_it->second,
              game_model_.results_format.columns[field_idx].ordering)) {
        return false;
      }
    }
    return true;
  };

  for (const auto& result : results.lobbies) {
    if (game_model_.results_format.columns.size() !=
        result.result_fields.size()) {
      LOG(ERROR) << "Result fields size mismatch: expected "
                 << game_model_.results_format.columns.size() << ", got "
                 << result.result_fields.size();
      continue;
    }

    if (match_result(result)) {
      matching_count++;
    }
  }

  return matching_count;
}

bool WxGamePage::ReselectLastSelectedRow() {
  if (!selected_result_id_) {
    return false;
  }

  for (int i = 0; i < results_list_->GetItemCount(); ++i) {
    wxVariant cellValue;
    results_list_->GetValue(cellValue, i, 0);

    if (cellValue.GetString() == *selected_result_id_) {
      results_list_->SelectRow(i);
      return true;
    }
  }

  return false;
}

void WxGamePage::BringConnectToLobbyDialog(wxDataViewItem selected_lobby) {
  if (!create_lobby_connector_) {
    return;
  }

  int row = results_list_->ItemToRow(selected_lobby);
  wxVariant value;

  results_list_->GetValue(value, row, 0);
  auto lobby_id = value.GetString().ToStdString();
  results_list_->GetValue(value, row, 1);
  auto game_mode = value.GetString().ToStdString();
  results_list_->GetValue(value, row, 2);
  auto owner = value.GetString().ToStdString();

  const bool show_as_modal = false;
  if (show_as_modal) {
    WxLobbyConnectDialog{
        parent_, lobby_id, game_mode, owner, create_lobby_connector_, true}
        .ShowModal();
  } else {
    auto* dialog = new WxLobbyConnectDialog(parent_, lobby_id, game_mode, owner,
                                            create_lobby_connector_, false);
    dialog->Show();
  }
}

std::optional<std::string> WxGamePage::GetLobbyMetadata(std::string lobby_id,
                                                        std::string key) const {
  auto lobby_metadata = lobbies_metadata_.find(lobby_id);
  if (lobby_metadata == lobbies_metadata_.end()) {
    return {};
  }

  auto value = lobby_metadata->second.find(key);
  if (value == lobby_metadata->second.end()) {
    return {};
  }

  return value->second;
}

}  // namespace ui::wx
