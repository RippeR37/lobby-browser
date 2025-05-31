#pragma once

#include <map>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

namespace model {

struct StartupConfig {
  std::vector<std::string> enabled_games;
  std::string selected_game;
  bool search_on_startup = false;
};

struct AppConfig {
  StartupConfig startup;
};

using GameConfig = nlohmann::json;
using UiConfig = nlohmann::json;

struct Config {
  AppConfig app;
  std::map<std::string, GameConfig> games;
  std::map<std::string, UiConfig> ui;
};

struct InitialConfigInput {
  std::vector<std::string> supported_games;
};

struct InitialConfig {
  std::vector<std::string> enabled_games;
};

}  // namespace model
