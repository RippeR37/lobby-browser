#pragma once

#include <vector>

#include "wx/dataview.h"

#include "models/game.h"

namespace ui::wx {

class WxResultsListModel : public wxDataViewListStore {
 public:
  WxResultsListModel(std::vector<model::GameResultsColumnFormat> formats);
  ~WxResultsListModel() override;

  // wxDataViewListStore
  int Compare(const wxDataViewItem& item1,
              const wxDataViewItem& item2,
              unsigned int column,
              bool ascending) const override;

 private:
  std::vector<model::GameResultsColumnFormat> formats_;
};

}  // namespace ui::wx
