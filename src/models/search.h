#pragma once

#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"

#include "models/game.h"

namespace model {

struct SearchRequest {
  std::string game_name;
  GameFilters search_filters;
  base::OnceCallback<void(GamePlayersResults)> players_callback;
};

enum SearchResult {
  kOk,
  kAborted,
  kError,
  kUnsupported,
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
    std::string profile_url;
  };

  bool all_members_known;
  std::vector<Member> members;
};

struct SearchUsersRequest {
  std::string game_name;
  std::string user_name;
};

struct SearchUserEntry {
  std::string user_name;
  std::vector<std::pair<std::string, std::string>> user_data;
};

struct SearchUsersResponse {
  SearchResult result;
  std::string error;
  std::vector<SearchUserEntry> results;
};

}  // namespace model
