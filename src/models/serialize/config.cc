#include "models/serialize/config.h"

namespace model {

void to_json(nlohmann::json& out, const StartupConfig& obj) {
  out = nlohmann::json{
      {"enabled_games", obj.enabled_games},
      {"selected_game", obj.selected_game},
      {"search_on_startup", obj.search_on_startup},
  };
}

void from_json(const nlohmann::json& out, StartupConfig& obj) {
  obj.enabled_games = out.value("enabled_games", std::vector<std::string>{});
  obj.selected_game = out.value("selected_game", std::string{});
  obj.search_on_startup = out.value("search_on_startup", false);
}

void to_json(nlohmann::json& out, const AppConfig& obj) {
  out = nlohmann::json{
      {"startup", obj.startup},
  };
}

void from_json(const nlohmann::json& out, AppConfig& obj) {
  obj.startup = out.value("startup", StartupConfig{});
}

void to_json(nlohmann::json& out, const Config& obj) {
  out = nlohmann::json{
      {"app", obj.app},
      {"games", obj.games},
      {"ui", obj.ui},
  };
}

void from_json(const nlohmann::json& out, Config& obj) {
  obj.app = out.value("app", AppConfig{});
  obj.games = out.value("games", std::map<std::string, nlohmann::json>{});
  obj.ui = out.value("ui", std::map<std::string, nlohmann::json>{});
}

}  // namespace model
