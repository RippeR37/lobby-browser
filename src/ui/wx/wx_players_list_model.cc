#include "ui/wx/wx_players_list_model.h"

namespace ui::wx {

WxPlayersListModel::WxPlayersListModel(unsigned int favorites_column)
    : favorites_column_(favorites_column) {}

WxPlayersListModel::~WxPlayersListModel() = default;

int WxPlayersListModel::Compare(const wxDataViewItem& item1,
                                const wxDataViewItem& item2,
                                unsigned int column,
                                bool ascending) const {
  if (column != favorites_column_) {
    wxVariant variant1, variant2;
    GetValue(variant1, item1, favorites_column_);
    GetValue(variant2, item2, favorites_column_);

    if (variant1.GetString().empty() != variant2.GetString().empty()) {
      return variant1.GetString().empty() ? 1 : -1;
    }
  }

  wxVariant variant1, variant2;
  GetValue(variant1, item1, column);
  GetValue(variant2, item2, column);

  auto value1 = variant1.GetString().Lower();
  auto value2 = variant2.GetString().Lower();

  return value1.CmpNoCase(value2) * (ascending ? 1 : -1);
}

bool WxPlayersListModel::GetAttr(const wxDataViewItem& item,
                                 unsigned int col,
                                 wxDataViewItemAttr& attr) const {
  auto result = wxDataViewListStore::GetAttr(item, col, attr);

  wxVariant value;
  GetValue(value, item, 2);
  if (!value.GetString().IsEmpty()) {
    attr.SetBold(true);
  }

  return result;
}

}  // namespace ui::wx
