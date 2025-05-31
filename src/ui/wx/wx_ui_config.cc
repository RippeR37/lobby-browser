#include "ui/wx/wx_ui_config.h"

namespace ui::wx {

void to_json(nlohmann::json& out, const UiInitialWindowPosition& obj) {
  out = {
      {"x", obj.x},
      {"y", obj.y},
  };
}

void from_json(const nlohmann::json& in, UiInitialWindowPosition& obj) {
  obj.x = in.value("x", -1);
  obj.y = in.value("y", -1);
}

void to_json(nlohmann::json& out, const UiStartupConfig& obj) {
  out = {
      {"initial_position", obj.initial_position},
  };
}

void from_json(const nlohmann::json& in, UiStartupConfig& obj) {
  obj.initial_position =
      in.value("initial_position", UiInitialWindowPosition{});
}

void to_json(nlohmann::json& out, const UiGameView& obj) {
  out = {
      {"sort_by_column", obj.sort_by_column},
      {"order_ascending", obj.order_ascending},
  };
}

void from_json(const nlohmann::json& in, UiGameView& obj) {
  obj.sort_by_column = in.value("sort_by_column", -1);
  obj.order_ascending = in.value("order_ascending", false);
}

void to_json(nlohmann::json& out, const UiGameConfig& obj) {
  out = {
      {"view", obj.view},
  };
}

void from_json(const nlohmann::json& in, UiGameConfig& obj) {
  obj.view = in.value("view", UiGameView{});
}

void to_json(nlohmann::json& out, const UiConfig& obj) {
  out = {
      {"startup", obj.startup},
      {"games", obj.games},
  };
}

void from_json(const nlohmann::json& in, UiConfig& obj) {
  obj.startup = in.value("startup", UiStartupConfig{});
  obj.games = in.value("games", std::map<std::string, UiGameConfig>{});
}

}  // namespace ui::wx
