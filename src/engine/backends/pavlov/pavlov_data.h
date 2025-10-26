#pragma once

#include <string>
#include <vector>

#include "nlohmann/json.hpp"

namespace engine::backend::pavlov {

//
// SearchUsers
//

struct SearchUsersRequest {
  std::string query;
};

struct SearchUsersPlayer {
  std::string display_name;
  std::string id;
  std::string platform;
  std::string platform_id;
};

struct SearchUsersResponse {
  std::vector<SearchUsersPlayer> players;
};

void to_json(nlohmann::json& out, const SearchUsersRequest& obj);
void from_json(const nlohmann::json& in, SearchUsersPlayer& obj);
void from_json(const nlohmann::json& in, SearchUsersResponse& obj);

}  // namespace engine::backend::pavlov
