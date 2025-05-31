#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace engine::backend::modio {

struct ModData {
  int64_t id;
  int64_t game_id;
  int64_t status;
  int64_t visible;
  int64_t date_added;
  int64_t date_updated;
  int64_t date_live;
  std::string name;
  std::string name_id;
  std::string summary;
  std::string description;
};

struct ModListingPage {
  std::vector<ModData> data;
  int64_t result_count = 0;
  int64_t result_offset = 0;
  int64_t result_limit = 0;
  int64_t result_total = 0;
};

}  // namespace engine::backend::modio
