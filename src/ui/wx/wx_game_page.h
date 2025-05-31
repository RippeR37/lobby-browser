#pragma once

#include <optional>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "wx/dataview.h"
#include "wx/notebook.h"
#include "wx/panel.h"

#include "models/game.h"
#include "models/search.h"
#include "ui/event_handler.h"
#include "utils/timer.h"

namespace ui::wx {

class WxGamePage {
 public:
  struct ColumnSortInfo {
    int column = -1;
    bool ascending = false;
  };

  WxGamePage(wxWindow* parent,
             EventHandler* event_handler,
             model::Game game_model,
             base::RepeatingCallback<void(size_t)> on_autosearch_found);
  ~WxGamePage();

  wxPanel* GetMainPanel() const;
  model::Game GetGameModel() const;
  model::GameFilters GetCurrentGameFilters() const;
  ColumnSortInfo GetColumnSortInfo() const;
  void SetColumnSorting(ColumnSortInfo column_sorting);
  void TriggerSearchIfPossible();
  void StartAutoSearch(std::map<std::string, std::string> autosearch_options);
  void StopAutoSearch();

 private:
  wxPanel* CreateMainGamePage(wxWindow* parent);
  wxDataViewCtrl* CreateMainGamePageLobbyListCtrl(
      wxWindow* parent,
      const model::GameResultsFormat& game_results_format);
  wxPanel* CreateMainGamePageDetailsPanel(wxWindow* parent);
  wxPanel* CreateMainGamePageDetailsFilterPanel(wxWindow* parent);
  wxPanel* CreateMainGamePageDetailsDetailsPanel(wxWindow* parent);

  void OnRowEntered(wxDataViewEvent& event);
  void OnSearchLobbiesAndServersDone(model::SearchResponse response);
  void RequestSelectedLobbyDetails(bool wait_for_full_details);
  void OnServerLobbyDetailsReceived(std::string result_id,
                                    model::SearchDetailsResponse details);
  void RefreshResultsList();
  size_t ResultsMatchingAutoSearchOptions(
      const model::GameSearchResults& results) const;
  bool ReselectLastSelectedRow();

  EventHandler* event_handler_;
  model::Game game_model_;
  base::RepeatingCallback<void(size_t)> on_autosearch_found_;

  wxPanel* main_panel_;
  wxDataViewListCtrl* results_list_;
  wxNotebook* game_details_notebook_;
  wxPanel* filters_panel_;
  wxButton* search_button_;

  model::GameSearchResults last_response_results_;
  std::optional<std::string> selected_result_id_;
  bool switch_to_details_tab_;

  std::optional<std::map<std::string, std::string>> autosearch_options_;
  util::Timer autosearch_timer_;

  base::WeakPtr<WxGamePage> weak_this_;
  base::WeakPtrFactory<WxGamePage> weak_factory_;
};

}  // namespace ui::wx
