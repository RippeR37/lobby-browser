#include "ui/wx/wx_theme.h"

#include "wx/pen.h"

namespace ui::wx {

namespace {
namespace light {
static const inline auto COLOR_MAIN_WINDOW_BG = wxColour{0xFA, 0xFA, 0xFA};
static const inline auto COLOR_MAIN_PANEL_BG = COLOR_MAIN_WINDOW_BG;
static const inline auto COLOR_TOOLBOOK_BG = COLOR_MAIN_WINDOW_BG;
static const inline auto COLOR_LIST_LOADING_BG = wxColour{0xEE, 0xEE, 0xEE};
static const inline auto COLOR_LIST_LOADED_BG = wxColour{0xFF, 0xFF, 0xFF};
static const inline auto COLOR_GAME_PANEL_BG = wxColour{0xFF, 0xFF, 0xFF};
static const inline auto COLOR_GAME_PAGE_BG = COLOR_GAME_PANEL_BG;
static const inline auto COLOR_GAME_PLAYERS_PAGE_BG = COLOR_GAME_PANEL_BG;
static const inline auto COLOR_GAME_DETAILS_BG = *wxWHITE;
}  // namespace light

namespace dark {
static const inline auto COLOR_MAIN_WINDOW_BG = wxColour{0x20, 0x20, 0x20};
static const inline auto COLOR_MAIN_PANEL_BG = COLOR_MAIN_WINDOW_BG;
static const inline auto COLOR_TOOLBOOK_BG = COLOR_MAIN_WINDOW_BG;
static const inline auto COLOR_LIST_LOADING_BG = wxColour{0x33, 0x33, 0x33};
static const inline auto COLOR_LIST_LOADED_BG = wxColour{0x2B, 0x2B, 0x2B};

static const inline auto COLOR_GAME_PANEL_BG = wxColour{0x20, 0x20, 0x20};

static const inline auto COLOR_GAME_PAGE_BG = wxColour{0x2B, 0x2B, 0x2B};
static const inline auto COLOR_GAME_PLAYERS_PAGE_BG = COLOR_GAME_PAGE_BG;
static const inline auto COLOR_GAME_DETAILS_BG = wxColour{0x2B, 0x2B, 0x2B};
}  // namespace dark
}  // namespace

WxThemeColors GetThemeColors(UiTheme theme) {
  if (theme == UiTheme::System) {
    if (wxSystemSettings::GetAppearance().AreAppsDark()) {
      theme = UiTheme::Dark;
    } else {
      theme = UiTheme::Light;
    }
  }

  if (theme == UiTheme::Light) {
    return WxThemeColors{
        // clang-format off
        UiEffectiveTheme::Light,
        light::COLOR_MAIN_WINDOW_BG,
        light::COLOR_MAIN_PANEL_BG,
        light::COLOR_TOOLBOOK_BG ,
        light::COLOR_LIST_LOADING_BG,
        light::COLOR_LIST_LOADED_BG,
        light::COLOR_GAME_PANEL_BG,
        light::COLOR_GAME_PAGE_BG,
        light::COLOR_GAME_PLAYERS_PAGE_BG,
        light::COLOR_GAME_DETAILS_BG,
        // clang-format on
    };
  } else {
    return WxThemeColors{
        // clang-format off
        UiEffectiveTheme::Dark,
        dark::COLOR_MAIN_WINDOW_BG,
        dark::COLOR_MAIN_PANEL_BG,
        dark::COLOR_TOOLBOOK_BG ,
        dark::COLOR_LIST_LOADING_BG,
        dark::COLOR_LIST_LOADED_BG,
        dark::COLOR_GAME_PANEL_BG,
        dark::COLOR_GAME_PAGE_BG,
        dark::COLOR_GAME_PLAYERS_PAGE_BG,
        dark::COLOR_GAME_DETAILS_BG,
        // clang-format on
    };
  }
};

wxColour WxDarkModeSettings::GetColour(wxSystemColour index) {
  switch (index) {
    case wxSYS_COLOUR_MENU:
      return wxColour{0x10, 0x10, 0x10};
    case wxSYS_COLOUR_BTNFACE:
      return wxColour{0x20, 0x20, 0x20};

    default:
      return wxDarkModeSettings::GetColour(index);
  }
}

wxPen WxDarkModeSettings::GetBorderPen() {
  return wxPen{wxColour{0x66, 0x66, 0x66}};
}

}  // namespace ui::wx
