#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace engine::backend::eos {

//
// SearchLobbies
//

enum class CriteriaOperator {
  EQUAL,
  NOT_EQUAL,
  ANY_OF,
};

struct SearchLobbiesCriteria {
  std::string key;
  CriteriaOperator op;
  std::variant<int64_t, std::string, std::vector<std::string>> value;
};

struct SearchLobbiesRequest {
  std::vector<SearchLobbiesCriteria> criteria;
  int64_t min_current_players;
  int64_t max_results;
};

struct SearchLobbiesSessionSettings {
  int64_t max_public_players;
  // ...
};

struct SearchLobbiesSession {
  std::string deployment;
  std::string id;
  std::string bucket;
  int64_t total_players;
  int64_t open_public_players;
  SearchLobbiesSessionSettings settings;
  bool started;
  std::map<std::string, std::string> attributes;
  std::string owner;
  int64_t owner_platform_id;
};

struct SearchLobbiesResponse {
  std::vector<SearchLobbiesSession> sessions;
  int64_t count;
};

}  // namespace engine::backend::eos
