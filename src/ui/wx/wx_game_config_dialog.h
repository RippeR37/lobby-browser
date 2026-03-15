#pragma once

#include <wx/bookctrl.h>
#include <wx/listctrl.h>
#include <wx/propdlg.h>
#include <wx/filepicker.h>
#include <wx/wx.h>

#include "models/game_config.h"
#include "ui/event_handler.h"

namespace ui::wx {

class WxGameConfigDialog : public wxPropertySheetDialog {
 public:
  WxGameConfigDialog(wxWindow* parent,
                     EventHandler* event_handler,
                     std::string game_name,
                     model::GameConfigDescriptor descriptor);

  void OnOK(wxCommandEvent& event);

 private:
  wxPanel* CreateSectionPage(wxWindow* parent, const model::GameConfigSection& section);
  wxWindow* CreateOptionControl(wxWindow* parent, const model::GameConfigOption& option);
  wxPanel* CreateListControl(wxWindow* parent, const model::GameConfigOption& option);

  EventHandler* event_handler_;
  std::string game_name_;
  model::GameConfigDescriptor descriptor_;
};

}  // namespace ui::wx
