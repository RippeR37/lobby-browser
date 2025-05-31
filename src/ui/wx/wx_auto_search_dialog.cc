#include "ui/wx/wx_auto_search_dialog.h"

namespace ui::wx {

WxAutoSearchDialog::WxAutoSearchDialog(wxWindow* parent,
                                       model::GameResultsFormat results_format)
    : wxDialog(parent, wxID_ANY, "Auto Search filters", wxDefaultPosition) {
  auto* main_sizer = new wxBoxSizer(wxVERTICAL);
  auto* button_sizer = new wxFlexGridSizer(2, 5, 5);
  button_sizer->AddGrowableCol(1);

  for (const auto& result_column : results_format.columns) {
    if (!result_column.visible) {
      continue;
    }

    if (!result_column.filter_in_auto_search) {
      continue;
    }

    switch (result_column.ordering) {
      case model::GameResultsColumnOrdering::kString:
      case model::GameResultsColumnOrdering::kPingOrString:
        AddFilter(button_sizer, result_column.name, false);
        break;

      case model::GameResultsColumnOrdering::kNumber:
      case model::GameResultsColumnOrdering::kNumberDivNumber:
        AddFilter(button_sizer, result_column.name, true, true);
        break;
    }
  }

  auto* action_sizer = new wxBoxSizer(wxHORIZONTAL);
  action_sizer->Add(new wxButton(this, wxID_OK), 0, wxALL, 5);
  action_sizer->Add(new wxButton(this, wxID_CANCEL), 0, wxALL, 5);
  main_sizer->Add(button_sizer, 1, wxEXPAND);

  main_sizer->Add(action_sizer, 0, wxALIGN_RIGHT);
  SetSizerAndFit(main_sizer);

  SetSize(wxSize{400, -1});
  Centre();
}

WxAutoSearchDialog::~WxAutoSearchDialog() = default;

std::map<std::string, std::string> WxAutoSearchDialog::GetFilterOptions()
    const {
  std::map<std::string, std::string> result;
  for (const auto& [name, option] : filter_options_) {
    if (option.check_box->IsChecked()) {
      if (option.num_ctrl) {
        result[name] = std::to_string(option.num_ctrl->GetValue());
      } else if (option.text_ctrl) {
        result[name] = option.text_ctrl->GetValue().ToStdString();
      }
    }
  }
  return result;
}

void WxAutoSearchDialog::AddFilter(wxFlexGridSizer* sizer,
                                   const std::string& label,
                                   bool numeric,
                                   bool enabled_by_default) {
  auto* check_box = new wxCheckBox(this, wxID_ANY, label);
  if (enabled_by_default) {
    check_box->SetValue(true);
  }
  check_box->Bind(wxEVT_CHECKBOX, &WxAutoSearchDialog::OnCheckToggle, this);
  sizer->Add(check_box, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);

  if (numeric) {
    auto* num_ctrl = new wxSpinCtrl(this, wxID_ANY);
    if (!enabled_by_default) {
      num_ctrl->Enable(false);
    }
    num_ctrl->SetRange(1, 100);
    sizer->Add(num_ctrl, 1, wxALL | wxEXPAND, 5);
    filter_options_[label] = {check_box, nullptr, num_ctrl};
  } else {
    auto* text_ctrl = new wxTextCtrl(this, wxID_ANY);
    if (!enabled_by_default) {
      text_ctrl->Enable(false);
    }
    sizer->Add(text_ctrl, 1, wxALL | wxEXPAND, 5);
    filter_options_[label] = {check_box, text_ctrl, nullptr};
  }
}

void WxAutoSearchDialog::OnCheckToggle(wxCommandEvent& event) {
  auto* source = dynamic_cast<wxCheckBox*>(event.GetEventObject());

  for (const auto& [name, option] : filter_options_) {
    if (option.check_box == source) {
      if (option.num_ctrl) {
        option.num_ctrl->Enable(source->IsChecked());
      }
      if (option.text_ctrl) {
        option.text_ctrl->Enable(source->IsChecked());
      }
      break;
    }
  }
}

}  // namespace ui::wx
