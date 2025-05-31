#pragma once

#include <map>
#include <string>

#include "wx/spinctrl.h"
#include "wx/wx.h"

#include "models/game.h"

namespace ui::wx {

class WxAutoSearchDialog : public wxDialog {
 public:
  WxAutoSearchDialog(wxWindow* parent, model::GameResultsFormat results_format);
  ~WxAutoSearchDialog() override;

  std::map<std::string, std::string> GetFilterOptions() const;

 private:
  void AddFilter(wxFlexGridSizer* sizer,
                 const std::string& label,
                 bool numeric,
                 bool enabled_by_default = false);
  void OnCheckToggle(wxCommandEvent& event);

  struct FilterOption {
    wxCheckBox* check_box;
    wxTextCtrl* text_ctrl;
    wxSpinCtrl* num_ctrl;
  };
  std::map<std::string, FilterOption> filter_options_;
};

}  // namespace ui::wx
