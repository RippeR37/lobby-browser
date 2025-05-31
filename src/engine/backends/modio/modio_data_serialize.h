#pragma once

#include "nlohmann/json.hpp"

#include "engine/backends/modio/modio_data.h"

namespace engine::backend::modio {

void to_json(nlohmann::json& out, const ModData& obj);
void to_json(nlohmann::json& out, const ModListingPage& obj);

void from_json(const nlohmann::json& in, ModData& obj);
void from_json(const nlohmann::json& in, ModListingPage& obj);

}  // namespace engine::backend::modio
