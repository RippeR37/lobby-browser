#pragma once

#include <string>

#include "models/search.h"

namespace ui::wx {

std::string WxWebViewMembersHeader();
std::string WxWebViewMembersMemberRow(
    const model::SearchDetailsResponse::Member& member);
std::string WxWebViewMembersFooter();

}  // namespace ui::wx
