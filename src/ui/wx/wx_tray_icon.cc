#include "ui/wx/wx_tray_icon.h"

#include "wx/msgdlg.h"

namespace ui::wx {

WxTrayIcon::WxTrayIcon(wxFrame* parent) : parent_(parent) {
  Bind(
      wxEVT_TASKBAR_LEFT_DOWN,
      [=](const wxTaskBarIconEvent&) { RestoreParent(); }, wxID_ANY);

  Bind(
      wxEVT_MENU, [=](const wxCommandEvent&) { RestoreParent(); },
      ID_TRAY_RESTORE);
  Bind(wxEVT_MENU, [=](const wxCommandEvent&) { QuitParent(); }, ID_TRAY_EXIT);
}

void WxTrayIcon::OnLeftButtonClick(wxTaskBarIconEvent&) {
  RestoreParent();
}

wxMenu* WxTrayIcon::CreatePopupMenu() {
  auto* menu = new wxMenu();
  menu->Append(ID_TRAY_RESTORE, "Restore");
  menu->Append(ID_TRAY_EXIT, "Quit");
  return menu;
}

void WxTrayIcon::RestoreParent() {
  if (!parent_) {
    return;
  }

  if (!parent_->IsShown()) {
    parent_->Show(true);
  }

  if (parent_->IsIconized()) {
    parent_->Restore();
  }

  parent_->Raise();
}

void WxTrayIcon::QuitParent() {
  wxCommandEvent event{wxEVT_MENU, wxID_EXIT};
  wxPostEvent(parent_, event);
}

}  // namespace ui::wx
