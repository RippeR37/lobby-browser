#include "ui/wx/wx_preferences_dialog.h"

namespace ui::wx {

WxPreferencesDialog::WxPreferencesDialog(wxWindow* parent, UiConfig* config)
    : config_(config) {
  Create(parent, wxID_ANY, "Preferences", wxDefaultPosition, wxSize(500, 400),
         wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

  CreateButtons(wxOK | wxCANCEL);

  wxBookCtrlBase* book = GetBookCtrl();

  book->AddPage(CreateGeneralPreferencesPage(book), "General");
  book->AddPage(CreateAppearancePreferencesPage(book), "Appearance");

  LayoutDialog();
}

int WxPreferencesDialog::ShowModal() {
  const auto result = wxPropertySheetDialog::ShowModal();

  if (result == wxID_OK && config_) {
    config_->startup.search_on_startup = search_on_startup_->IsChecked();
    config_->preferences.minimize_to_tray = minimize_to_tray_->IsChecked();
  }

  return result;
}

wxPanel* WxPreferencesDialog::CreateGeneralPreferencesPage(wxWindow* parent) {
  auto* panel = new wxPanel(parent);
  auto* sizer = new wxBoxSizer(wxVERTICAL);

  search_on_startup_ = new wxCheckBox(panel, wxID_ANY, "Search on startup");
  minimize_to_tray_ = new wxCheckBox(panel, wxID_ANY, "Minimize to tray");

  if (config_) {
    search_on_startup_->SetValue(config_->startup.search_on_startup);
    minimize_to_tray_->SetValue(config_->preferences.minimize_to_tray);
  }

  sizer->Add(search_on_startup_, 0, wxALL, 5);
  sizer->Add(minimize_to_tray_, 0, wxALL, 5);

  panel->SetSizer(sizer);
  return panel;
}

wxPanel* WxPreferencesDialog::CreateAppearancePreferencesPage(
    wxWindow* parent) {
  auto* panel = new wxPanel(parent);
  auto* sizer = new wxBoxSizer(wxHORIZONTAL);

  wxArrayString themes;
  themes.Add("Light");
  themes.Add("Dark");
  themes.Add("System");

  sizer->Add(new wxStaticText(panel, wxID_ANY, "Theme:"), 0, wxTOP | wxLEFT, 5);
  theme_choice_ =
      new wxChoice(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, themes);
  theme_choice_->SetSelection(0);
  theme_choice_->Disable();

  sizer->Add(theme_choice_, 0, wxALL, 5);

  panel->SetSizer(sizer);
  return panel;
}

}  // namespace ui::wx
