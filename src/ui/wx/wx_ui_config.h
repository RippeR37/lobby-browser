#pragma once

#include "nlohmann/json.hpp"

namespace ui::wx {

struct UiInitialWindowPosition {
  int x = 0;
  int y = 0;
};

struct UiStartupConfig {
  UiInitialWindowPosition initial_position;
};

struct UiGameView {
  int sort_by_column = -1;
  bool order_ascending = false;
};

struct UiGameConfig {
  UiGameView view;
};

struct UiConfig {
  UiStartupConfig startup;
  std::map<std::string, UiGameConfig> games;
};

//
// JSON
//

void to_json(nlohmann::json& out, const UiInitialWindowPosition& obj);
void from_json(const nlohmann::json& in, UiInitialWindowPosition& obj);

void to_json(nlohmann::json& out, const UiStartupConfig& obj);
void from_json(const nlohmann::json& in, UiStartupConfig& obj);

void to_json(nlohmann::json& out, const UiGameView& obj);
void from_json(const nlohmann::json& in, UiGameView& obj);

void to_json(nlohmann::json& out, const UiGameConfig& obj);
void from_json(const nlohmann::json& in, UiGameConfig& obj);

void to_json(nlohmann::json& out, const UiConfig& obj);
void from_json(const nlohmann::json& in, UiConfig& obj);

}  // namespace ui::wx
