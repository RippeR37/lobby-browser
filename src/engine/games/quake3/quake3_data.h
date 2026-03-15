#pragma once

#include <string>
#include <vector>
#include <set>

#include "nlohmann/json.hpp"
#include "models/game.h"

namespace engine::game::quake3 {

struct Quake3MasterServer {
  std::string name;
  std::string address;
  bool built_in = false;
  bool enabled = true;

  bool operator<(const Quake3MasterServer& other) const {
    return address < other.address;
  }
};

struct Quake3OtherFilters {
  bool hide_full = false;
  bool hide_empty = false;
};

struct Quake3GameModeFilters {
  bool all = true;
  bool ffa = true;
  bool tournament = true; // 1v1
  bool single = true;
  bool tdm = true;
  bool ctf = true;
  bool one_flag_ctf = true;
  bool overload = true;
  bool harvester = true;
  bool team_ffa = true;
};

struct Quake3PingFilters {
  bool all = true;
  bool max_50 = false;
  bool max_100 = false;
  bool max_200 = false;
};

struct Quake3Filters {
  std::vector<Quake3MasterServer> master_servers;
  Quake3GameModeFilters game_modes;
  Quake3PingFilters pings;
  Quake3OtherFilters others;
};

struct Quake3Config {
  Quake3Filters filters;
  std::string executable_path;
};

struct Quake3Server {
  std::string address; // IP:Port
  std::string hostname;
  std::string map;
  int players = 0;
  int max_players = 0;
  std::string game_mode;
  int ping = 999;
  std::string version;
};

//
// JSON
//

void to_json(nlohmann::json& out, const Quake3MasterServer& obj);
void from_json(const nlohmann::json& in, Quake3MasterServer& obj);

void to_json(nlohmann::json& out, const Quake3OtherFilters& obj);
void from_json(const nlohmann::json& in, Quake3OtherFilters& obj);

void to_json(nlohmann::json& out, const Quake3GameModeFilters& obj);
void from_json(const nlohmann::json& in, Quake3GameModeFilters& obj);

void to_json(nlohmann::json& out, const Quake3PingFilters& obj);
void from_json(const nlohmann::json& in, Quake3PingFilters& obj);

void to_json(nlohmann::json& out, const Quake3Filters& obj);
void from_json(const nlohmann::json& in, Quake3Filters& obj);

void to_json(nlohmann::json& out, const Quake3Config& obj);
void from_json(const nlohmann::json& in, Quake3Config& obj);

//
// Game model
//

model::GameFilters ToModel(const Quake3Filters& filters);
Quake3Filters FromModel(const model::GameFilters& filters);

}  // namespace engine::game::quake3
