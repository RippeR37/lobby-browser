#pragma once

#include <string>

#include "models/search.h"

#include "ui/wx/wx_theme.h"

namespace ui::wx {

std::string WxWebViewMembersHeader(UiEffectiveTheme theme,
                                   const std::string& game_name);

std::string WxWebViewMembersMemberRow(
    UiEffectiveTheme theme,
    const model::SearchDetailsResponse::Member& member,
    const std::string& game_name);

std::string WxWebViewMembersFooter();

}  // namespace ui::wx
