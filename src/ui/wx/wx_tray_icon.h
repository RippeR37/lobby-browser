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
  WxTrayIcon(wxFrame* parent);

  void OnLeftButtonClick(wxTaskBarIconEvent&);
  wxMenu* CreatePopupMenu() override;

 private:
  void RestoreParent();
  void QuitParent();

  wxFrame* parent_;
};

}  // namespace ui::wx
