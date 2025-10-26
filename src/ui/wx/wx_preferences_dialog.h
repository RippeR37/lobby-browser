#pragma once

#include <wx/bookctrl.h>
#include <wx/checklst.h>
#include <wx/choice.h>
#include <wx/propdlg.h>
#include <wx/wx.h>

#include "ui/wx/wx_ui_config.h"

namespace ui::wx {

class WxPreferencesDialog : public wxPropertySheetDialog {
 public:
  WxPreferencesDialog(wxWindow* parent, UiConfig* config);

  int ShowModal() override;

 private:
  wxPanel* CreateGeneralPreferencesPage(wxWindow* parent);
  wxPanel* CreateAppearancePreferencesPage(wxWindow* parent);

  UiConfig* config_;
  wxCheckBox* search_on_startup_ = nullptr;
  wxCheckBox* minimize_to_tray_ = nullptr;
  wxChoice* theme_choice_ = nullptr;
};

}  // namespace ui::wx
