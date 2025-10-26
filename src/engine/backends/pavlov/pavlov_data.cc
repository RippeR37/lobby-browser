#include "engine/backends/pavlov/pavlov_data.h"

namespace engine::backend::pavlov {

void to_json(nlohmann::json& out, const SearchUsersRequest& obj) {
  out = {
      {"query", obj.query},
  };
}

void from_json(const nlohmann::json& in, SearchUsersPlayer& obj) {
  obj.display_name = in.value("display_name", "");
  obj.id = in.value("id", "");
  obj.platform = in.value("platform", "");
  obj.platform_id = in.value("platform_id", "");
}

void from_json(const nlohmann::json& in, SearchUsersResponse& obj) {
  obj.players = in.value("players", std::vector<SearchUsersPlayer>{});
}

}  // namespace engine::backend::pavlov
