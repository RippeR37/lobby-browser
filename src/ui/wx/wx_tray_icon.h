#pragma once

#include "wx/frame.h"
#include "wx/menu.h"
#include "wx/taskbar.h"

namespace ui::wx {

enum {
  ID_TRAY_ICON = wxID_HIGHEST + 1,
  ID_TRAY_RESTORE,
  ID_TRAY_EXIT,
};

class WxTrayIcon : public wxTaskBarIcon {
 public:
  class Client {
   public:
    virtual ~Client() = default;

    virtual void RestoreFromTray() = 0;
    virtual void QuitFromTray() = 0;
  };

  WxTrayIcon(Client* client);

  void OnLeftButtonClick(wxTaskBarIconEvent&);
  wxMenu* CreatePopupMenu() override;

 private:
  void RestoreClient();
  void QuitClient();

  Client* client_;
};

}  // namespace ui::wx
