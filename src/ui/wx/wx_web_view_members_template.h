#pragma once

#include <string>

#include "models/search.h"

#include "ui/wx/wx_theme.h"

namespace ui::wx {

std::string WxWebViewMembersHeader(UiEffectiveTheme theme);
std::string WxWebViewMembersMemberRow(
    const model::SearchDetailsResponse::Member& member);
std::string WxWebViewMembersFooter();

}  // namespace ui::wx
