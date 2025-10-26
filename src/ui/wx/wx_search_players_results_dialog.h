#pragma once

#include <vector>

#include "wx/wx.h"

#include "models/search.h"

namespace ui::wx {

class WxPlayerSearchResultsDialog : public wxDialog {
 public:
  WxPlayerSearchResultsDialog(
      wxWindow* parent,
      const std::vector<model::SearchUserEntry>& results);
};

}  // namespace ui::wx
