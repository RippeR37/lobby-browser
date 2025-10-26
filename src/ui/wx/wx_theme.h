#pragma once

#include "wx/msw/darkmode.h"

namespace ui::wx {

enum class UiTheme {
  Light,
  Dark,
  System,
};

enum class UiEffectiveTheme {
  Light,
  Dark,
};

struct WxThemeColors {
  UiEffectiveTheme effective_theme;

  wxColour MainWindowBg;
  wxColour MainPanelBg;
  wxColour GameToolbookBg;
  wxColour ListLoadingBg;
  wxColour ListLoadedBg;
  wxColour GamePanelBg;
  wxColour GamePageBg;
  wxColour GamePlayersPageBg;
  wxColour GameDetailsBg;
};

WxThemeColors GetThemeColors(UiTheme theme);

class WxDarkModeSettings : public wxDarkModeSettings {
 public:
  wxColour GetColour(wxSystemColour index) override;
  wxPen GetBorderPen() override;
};

}  // namespace ui::wx
