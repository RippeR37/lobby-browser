#include "engine/backends/modio/modio_data_serialize.h"

namespace engine::backend::modio {

void to_json(nlohmann::json& out, const ModData& obj) {
  out = {
      {"id", obj.id},
      {"game_id", obj.game_id},
      {"status", obj.status},
      {"visible", obj.visible},
      {"date_added", obj.date_added},
      {"date_updated", obj.date_updated},
      {"date_live", obj.date_live},
      {"name", obj.name},
      {"name_id", obj.name_id},
      {"summary", obj.summary},
      {"description", obj.description},
  };
}

void to_json(nlohmann::json& out, const ModListingPage& obj) {
  out = {
      {"data", obj.data},
      {"result_count", obj.result_count},
      {"result_offset", obj.result_offset},
      {"result_limit", obj.result_limit},
      {"result_total", obj.result_total},
  };
}

void from_json(const nlohmann::json& in, ModData& obj) {
  obj.id = in.value("id", -1);
  obj.game_id = in.value("game_id", -1);
  obj.status = in.value("status", -1);
  obj.visible = in.value("visible", -1);
  obj.date_added = in.value("date_added", -1);
  obj.date_updated = in.value("date_updated", -1);
  obj.date_live = in.value("date_live", -1);
  obj.name = in.value("name", "");
  obj.name_id = in.value("name_id", "");
  obj.summary = in.value("summary", "");
  obj.description = in.value("description", "");
}

void from_json(const nlohmann::json& in, ModListingPage& obj) {
  obj.data = in.value("data", std::vector<ModData>{});
  // Defaults set to 0 for safety
  obj.result_count = in.value("result_count", 0);
  obj.result_offset = in.value("result_offset", 0);
  obj.result_limit = in.value("result_limit", 0);
  obj.result_total = in.value("result_total", 0);
}

}  // namespace engine::backend::modio
