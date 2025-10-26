#pragma once

#include "wx/dataview.h"

namespace ui::wx {

class WxPlayersListModel : public wxDataViewListStore {
 public:
  WxPlayersListModel(unsigned int favorites_column);
  ~WxPlayersListModel() override;

  // wxDataViewListStore
  int Compare(const wxDataViewItem& item1,
              const wxDataViewItem& item2,
              unsigned int column,
              bool ascending) const override;
  bool GetAttr(const wxDataViewItem& item,
               unsigned int col,
               wxDataViewItemAttr& attr) const override;

 private:
  unsigned int favorites_column_;
};

}  // namespace ui::wx
