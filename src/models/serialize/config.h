#include "models/config.h"

#include "nlohmann/json.hpp"

namespace model {

void to_json(nlohmann::json& out, const StartupConfig& obj);
void from_json(const nlohmann::json& out, StartupConfig& obj);

void to_json(nlohmann::json& out, const AppConfig& obj);
void from_json(const nlohmann::json& out, AppConfig& obj);

void to_json(nlohmann::json& out, const Config& obj);
void from_json(const nlohmann::json& out, Config& obj);

}  // namespace model
