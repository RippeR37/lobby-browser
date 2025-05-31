#pragma once

#include <string>
#include <vector>

#include "base/callback.h"

#include "models/game.h"

namespace model {

struct SearchRequest {
  std::string game_name;
  GameFilters search_filters;
};

enum SearchResult {
  kOk,
  kAborted,
  kError,
};

struct SearchResponse {
  SearchResult result;
  std::string error;
  GameSearchResults results;
};

struct SearchDetailsRequest {
  std::string game_name;
  std::string result_id;
  bool wait_for_full_details;
};

struct SearchDetailsResponse {};

}  // namespace model
