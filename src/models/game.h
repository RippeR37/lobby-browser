#pragma once

#include <string>
#include <vector>

#include "base/callback.h"

namespace model {

struct GameFilterOption {
  std::string name;
  bool enabled;
  bool refresh_on_change = false;
};

struct GameFilterGroup {
  std::string name;
  std::vector<GameFilterOption> filter_options;
};

enum class GameResultsColumnOrdering {
  kString,
  kNumber,
  kNumberDivNumber,
  kPingOrString,
};

enum class GameResultsColumnAlignment {
  kLeft,
  kCenter,
  kRight,
};

struct GameResultsColumnFormat {
  std::string name;
  bool visible;
  int64_t proportion;
  GameResultsColumnAlignment alignment;
  GameResultsColumnOrdering ordering;
  bool filter_in_auto_search = false;
};

struct GameResultsFormat {
  std::vector<GameResultsColumnFormat> columns;
  bool has_lobby_details;
};

using GameFilters = std::vector<GameFilterGroup>;

struct GameServerLobbyResult {
  std::vector<std::string> result_fields;
};

using GameSearchResults = std::vector<GameServerLobbyResult>;
using GameSearchResultsFilterCallback =
    base::RepeatingCallback<GameSearchResults(const GameSearchResults&,
                                              const GameFilters&)>;

struct Game {
  std::string name;
  std::string icon_path;
  GameFilters filter_groups;
  GameResultsFormat results_format;
  GameSearchResultsFilterCallback results_filter_callback;
  // ...
};

}  // namespace model
