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

struct SearchDetailsResponse {
  struct Member {
    std::string id;
    std::string name;
    std::string avatar_url;
    std::string icon_url;
  };

  bool all_members_known;
  std::vector<Member> members;
};

}  // namespace model
