#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "base/callback.h"

namespace model {

struct GameFeatures {
  bool has_lobby_details;
  bool search_players;
  bool list_players;
  bool connect_to_lobby;
};

struct GameFilterOption {
  std::string name;
  bool enabled;
  bool refresh_on_change = false;
  bool disables_siblings = false;
};

enum class GameFilterGroupType {
  kCheckbox,
  kRadioButton,
};

struct GameFilterGroup {
  std::string name;
  std::vector<GameFilterOption> filter_options;
  GameFilterGroupType type = GameFilterGroupType::kCheckbox;
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
  std::vector<GameResultsColumnFormat> players_columns;
};

using GameFilters = std::vector<GameFilterGroup>;

struct GameServerLobbyResult {
  std::vector<std::string> result_fields;
  std::map<std::string, std::string> metadata;
};

using GameSearchLobbiesResults = std::vector<GameServerLobbyResult>;

struct GamePlayersResult {
  std::vector<std::string> result_fields;
};

using GamePlayersResults = std::vector<GamePlayersResult>;

struct GameSearchResults {
  GameSearchLobbiesResults lobbies;
};

using GameSearchResultsFilterCallback =
    base::RepeatingCallback<GameSearchResults(const GameSearchResults&,
                                              const GameFilters&)>;

struct Game {
  std::string name;
  std::string icon_path;
  GameFeatures features;
  GameFilters filter_groups;
  GameResultsFormat results_format;
  GameSearchResultsFilterCallback results_filter_callback;
  // ...
};

}  // namespace model
