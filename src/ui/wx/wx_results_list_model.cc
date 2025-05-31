#include "ui/wx/wx_results_list_model.h"

namespace ui::wx {

WxResultsListModel::WxResultsListModel(
    std::vector<model::GameResultsColumnFormat> formats)
    : formats_(std::move(formats)) {}

WxResultsListModel::~WxResultsListModel() = default;

int WxResultsListModel::Compare(const wxDataViewItem& item1,
                                const wxDataViewItem& item2,
                                unsigned int column,
                                bool ascending) const {
  int result = 0;

  if (column >= formats_.size()) {
    result = wxDataViewListStore::Compare(item1, item2, column, ascending);
  } else {
    switch (formats_[column].ordering) {
      case model::GameResultsColumnOrdering::kString:
        result = wxDataViewListStore::Compare(item1, item2, column, ascending);
        break;

      case model::GameResultsColumnOrdering::kNumber: {
        wxVariant variant1, variant2;
        GetValue(variant1, item1, column);
        GetValue(variant2, item2, column);

        auto value1 = std::stoi(variant1.GetString().ToStdString());
        auto value2 = std::stoi(variant2.GetString().ToStdString());

        result = ascending ? value1 - value2 : value2 - value1;
        break;
      }

      case model::GameResultsColumnOrdering::kNumberDivNumber: {
        wxVariant variant1, variant2;
        GetValue(variant1, item1, column);
        GetValue(variant2, item2, column);

        auto value1 = variant1.GetString().ToStdString();
        auto value2 = variant2.GetString().ToStdString();

        int value1a = 0, value1b = 0, value2a = 0, value2b = 0;
        if (std::sscanf(value1.c_str(), "%d/%d", &value1a, &value1b) == 2 &&
            std::sscanf(value2.c_str(), "%d/%d", &value2a, &value2b) == 2) {
          if (value1a != value2a) {
            result = ascending ? value1a - value2a : value2a - value1a;
          } else {
            result = ascending ? value1b - value2b : value2b - value1b;
          }
        } else {
          result =
              wxDataViewListStore::Compare(item1, item2, column, ascending);
        }
        break;
      }

      case model::GameResultsColumnOrdering::kPingOrString:
        wxVariant variant1, variant2;
        GetValue(variant1, item1, column);
        GetValue(variant2, item2, column);

        auto value1 = variant1.GetString().ToStdString();
        auto value2 = variant2.GetString().ToStdString();

        int ping1 = 0, ping2 = 0;
        if (std::sscanf(value1.c_str(), "%dms", &ping1) == 1 &&
            std::sscanf(value2.c_str(), "%dms", &ping2) == 1) {
          result = ascending ? ping1 - ping2 : ping2 - ping1;
        } else {
          result =
              wxDataViewListStore::Compare(item1, item2, column, ascending);
        }
        break;
    }
  }

  // If there is a tie, provide stable ordering by using first column as tie
  // breaker.
  if (result == 0 && column != 0 && !formats_.empty()) {
    return Compare(item1, item2, 0, ascending);
  }

  return result;
}

}  // namespace ui::wx
