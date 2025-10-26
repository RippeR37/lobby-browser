#include "ui/wx/wx_search_players_results_dialog.h"

#include "wx/grid.h"

namespace ui::wx {

WxPlayerSearchResultsDialog::WxPlayerSearchResultsDialog(
    wxWindow* parent,
    const std::vector<model::SearchUserEntry>& results)
    : wxDialog(parent,
               wxID_ANY,
               "Player search results",
               wxDefaultPosition,
               wxSize(500, 300)) {
  if (results.empty()) {
    return;
  }

  auto* sizer = new wxBoxSizer(wxVERTICAL);

  auto* grid = new wxGrid(this, wxID_ANY);
  grid->CreateGrid(static_cast<int>(results.size()),
                   static_cast<int>(results.front().user_data.size()) + 1);

  grid->SetColLabelValue(0, "Player");
  for (size_t i = 0; i < results.front().user_data.size(); ++i) {
    grid->SetColLabelValue(static_cast<int>(i) + 1,
                           results.front().user_data[i].first);
  }

  for (int i = 0; static_cast<size_t>(i) < results.size(); ++i) {
    const auto& result = results[i];
    grid->SetCellValue(i, 0, result.user_name);

    for (int j = 0; static_cast<size_t>(j) < result.user_data.size(); ++j) {
      const auto& field = result.user_data[j];
      grid->SetCellValue(i, j + 1, field.second);
    }
  }

  grid->AutoSize();

  grid->EnableEditing(false);
  grid->DisableDragColSize();
  grid->DisableDragRowSize();
  grid->DisableDragColMove();
  grid->DisableDragGridSize();

  sizer->Add(grid, 1, wxEXPAND | wxALL, 10);

  // OK button to close
  auto* okButton = new wxButton(this, wxID_OK, "OK");
  sizer->Add(okButton, 0, wxALIGN_CENTER | wxBOTTOM, 10);

  SetSizerAndFit(sizer);
  Centre();
}

}  // namespace ui::wx
