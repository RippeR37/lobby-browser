#include "ui/wx/wx_tray_icon.h"

#include "wx/msgdlg.h"

namespace ui::wx {

WxTrayIcon::WxTrayIcon(Client* client) : client_(client) {
  Bind(
      wxEVT_TASKBAR_LEFT_DOWN,
      [=, this](const wxTaskBarIconEvent&) { RestoreClient(); }, wxID_ANY);

  Bind(
      wxEVT_MENU, [=, this](const wxCommandEvent&) { RestoreClient(); },
      ID_TRAY_RESTORE);
  Bind(
      wxEVT_MENU, [=, this](const wxCommandEvent&) { QuitClient(); },
      ID_TRAY_EXIT);
}

void WxTrayIcon::OnLeftButtonClick(wxTaskBarIconEvent&) {
  RestoreClient();
}

wxMenu* WxTrayIcon::CreatePopupMenu() {
  auto* menu = new wxMenu();
  menu->Append(ID_TRAY_RESTORE, "Restore");
  menu->Append(ID_TRAY_EXIT, "Quit");
  return menu;
}

void WxTrayIcon::RestoreClient() {
  if (!client_) {
    return;
  }

  client_->RestoreFromTray();
}

void WxTrayIcon::QuitClient() {
  if (!client_) {
    return;
  }

  client_->QuitFromTray();
}

}  // namespace ui::wx
