#include "ui/wx/wx_game_page.h"

#include "base/logging.h"
#include "wx/bmpbuttn.h"
#include "wx/checkbox.h"
#include "wx/itemattr.h"
#include "wx/sizer.h"
#include "wx/statbox.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"

#include "ui/wx/wx_results_list_model.h"
#include "utils/strings.h"

namespace ui::wx {

namespace {

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

}  // namespace

WxGamePage::WxGamePage(
    wxWindow* parent,
    EventHandler* event_handler,
    model::Game game_model,
    base::RepeatingCallback<void(size_t)> on_autosearch_found)
    : event_handler_(event_handler),
      game_model_(std::move(game_model)),
      on_autosearch_found_(std::move(on_autosearch_found)),
      main_panel_(new wxPanel(parent)),
      results_list_(nullptr),
      game_details_notebook_(nullptr),
      filters_panel_(nullptr),
      search_button_(nullptr),
      switch_to_details_tab_(false),
      weak_factory_(this) {
  weak_this_ = weak_factory_.GetWeakPtr();

  auto* main_panel_notebook = new wxNotebook(main_panel_, wxID_ANY);
  main_panel_notebook->SetBackgroundColour(wxColour{0xFA, 0xFA, 0xFA});

  // Add pages to the inner notebook
  main_panel_notebook->AddPage(CreateMainGamePage(main_panel_notebook),
                               "Lobbies and Servers");

  // Layout for the toolbook page
  auto* panelSizer = new wxBoxSizer(wxVERTICAL);
  panelSizer->Add(main_panel_notebook, 1, wxEXPAND);
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

  results_list_->SetBackgroundColour(wxColour{0xEE, 0xEE, 0xEE});
  results_list_->Refresh();

  auto current_filters = GetCurrentGameFilters();
  event_handler_->OnSearchLobbiesAndServers(
      model::SearchRequest{
          game_model_.name,
          std::move(current_filters),
      },
      base::BindOnce(&WxGamePage::OnSearchLobbiesAndServersDone, weak_this_));
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

wxPanel* WxGamePage::CreateMainGamePage(wxWindow* parent) {
  // First page of the notebook
  auto* main_game_page = new wxPanel(parent);
  main_game_page->SetBackgroundColour(*wxWHITE);

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

  results_list_->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED,
                      [=](wxDataViewEvent& event) { OnRowEntered(event); });

  return results_list_;
}

wxPanel* WxGamePage::CreateMainGamePageDetailsPanel(wxWindow* parent) {
  auto* details_panel = new wxPanel(parent);
  auto* details_panel_sizer = new wxBoxSizer(wxVERTICAL);
  details_panel->SetSizer(details_panel_sizer);

  // Notebook
  game_details_notebook_ = new wxNotebook(details_panel, wxID_ANY);
  game_details_notebook_->SetBackgroundColour(*wxWHITE);
  details_panel_sizer->Add(game_details_notebook_, 20, wxEXPAND);

  game_details_notebook_->AddPage(
      CreateMainGamePageDetailsFilterPanel(game_details_notebook_), "Filters");
  if (game_model_.results_format.has_lobby_details) {
    game_details_notebook_->AddPage(
        CreateMainGamePageDetailsDetailsPanel(game_details_notebook_),
        "Details");
  }

  // Action buttons
  search_button_ = new wxButton(details_panel, wxID_ANY, "Find Lobbies");
  details_panel_sizer->Add(search_button_, 0, wxALIGN_CENTER | wxTOP, 10);

  // Bind callback
  search_button_->Bind(wxEVT_BUTTON,
                       [=](wxCommandEvent&) { TriggerSearchIfPossible(); });

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
        checkbox->Bind(wxEVT_CHECKBOX,
                       [=](const wxCommandEvent&) { RefreshResultsList(); });
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

  // TODO: implement

  return panel;
}

void WxGamePage::OnRowEntered(wxDataViewEvent& event) {
  if (!game_model_.results_format.has_lobby_details) {
    return;
  }

  auto selected_row_item = event.GetItem();
  if (!selected_row_item.IsOk()) {
    return;
  }

  // Assume first is always ID, might need to be tweaked later?
  wxVariant id_variant;
  results_list_->GetStore()->GetValue(id_variant, selected_row_item, 0);

  selected_result_id_ = id_variant.GetString();
  switch_to_details_tab_ = true;

  // Don't want for full details as we want to present something as quickly as
  // possible. We will re-request full details if they aren't known yet when
  // we'll get partial ones.
  RequestSelectedLobbyDetails(false);
};

void WxGamePage::OnSearchLobbiesAndServersDone(model::SearchResponse response) {
  search_button_->Enable();
  results_list_->SetBackgroundColour(wxColour{0xFF, 0xFF, 0xFF});
  results_list_->Refresh();

  if (response.result != model::SearchResult::kOk) {
    LOG(ERROR) << __FUNCTION__
               << "() result: " << static_cast<int>(response.result) << " - "
               << response.error;
    return;
  }

  last_response_results_ = std::move(response.results);
  RefreshResultsList();

  if (selected_result_id_) {
    // List of known members might have changed, so let's re-request details.
    RequestSelectedLobbyDetails(false);
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

  if (game_details_notebook_->GetPageCount() >= 2 && switch_to_details_tab_) {
    game_details_notebook_->SetSelection(1);
    switch_to_details_tab_ = false;
  }

  // TODO: implement
  (void)details;
}

void WxGamePage::RefreshResultsList() {
  results_list_->DeleteAllItems();

  const auto filtered_results = game_model_.results_filter_callback.Run(
      last_response_results_, GetCurrentGameFilters());

  // Append new results
  for (const auto& result : filtered_results) {
    std::vector<wxString> item_values;
    for (const auto& field : result.result_fields) {
      item_values.emplace_back(wxString::FromUTF8(field));
    };

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

  for (const auto& result : results) {
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

}  // namespace ui::wx
