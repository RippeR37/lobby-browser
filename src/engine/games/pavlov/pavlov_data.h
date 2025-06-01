#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "models/game.h"

namespace engine::game::pavlov {

struct PavlovGameModeFilters {
  bool dm;
  bool tdm;
  bool snd;
  bool gun;
  bool ww2tdm;
  bool ww2gun;
  bool custom;
  bool ttt;
  bool oitc;
  bool hide;
  bool push;
  bool zombies;
  bool ph;
  bool infection;
  bool koth;
  bool all;

  bool AllEnabled() const;
  std::vector<std::string> ToVec() const;
};

struct PavlovHostRegions {
  bool america;
  bool europe;
  bool asia_pacific;

  bool AllEnabled() const;
  std::vector<std::string> ToVec() const;
};

struct PavlovHostModeFilters {
  bool lobby;
  bool server;
  bool crossplay;

  PavlovHostRegions regions;
};

struct PavlovOtherFilters {
  bool hide_full;
  bool hide_empty;
  bool hide_locked;
  bool only_compatible;
};

struct PavlovFilters {
  PavlovGameModeFilters game_modes;
  PavlovHostModeFilters host_modes;
  PavlovOtherFilters others;
};

struct PavlovConfig {
  std::string game_version;
  PavlovFilters filters;
};

struct PavlovServer {
  std::string name;
  int64_t slots;
  int64_t max_slots;
  std::string map_id;
  std::string map_label;
  int64_t port;
  bool password_protected;
  bool secured;
  std::string game_mode;
  std::string game_mode_label;
  std::string ip;
  std::string version;
  std::string updated_ts;
};

struct PavlovServerListResponse {
  std::string result;
  std::vector<PavlovServer> servers;
};

enum class PavlovPlatform {
  kPCVR,
  kPSVR2,
};

// Unified struct for both Lobby and Server data
struct PavlovLobbyServer {
  std::string id;
  std::string name_owner;
  int64_t players;
  int64_t max_players;
  std::string gamemode;
  std::string map;
  std::string map_label;
  bool crossplatform;
  bool locked;
  std::optional<PavlovPlatform> platform;
  std::string state;

  // Lobby specific
  std::string region;

  // Server specific
  std::string ip;
  int64_t port;
};

struct PavlovSearchResponse {
  bool success;
  std::string error;
  std::vector<PavlovLobbyServer> results;
};

//
// JSON
//

void to_json(nlohmann::json& out, const PavlovGameModeFilters& obj);
void from_json(const nlohmann::json& in, PavlovGameModeFilters& obj);

void to_json(nlohmann::json& out, const PavlovHostRegions& obj);
void from_json(const nlohmann::json& in, PavlovHostRegions& obj);

void to_json(nlohmann::json& out, const PavlovHostModeFilters& obj);
void from_json(const nlohmann::json& in, PavlovHostModeFilters& obj);

void to_json(nlohmann::json& out, const PavlovOtherFilters& obj);
void from_json(const nlohmann::json& in, PavlovOtherFilters& obj);

void to_json(nlohmann::json& out, const PavlovFilters& obj);
void from_json(const nlohmann::json& in, PavlovFilters& obj);

void to_json(nlohmann::json& out, const PavlovConfig& obj);
void from_json(const nlohmann::json& in, PavlovConfig& obj);

void from_json(const nlohmann::json& in, PavlovServer& obj);
void from_json(const nlohmann::json& in, PavlovServerListResponse& obj);

//
// Game model
//

model::GameFilters ToModel(const PavlovFilters& filters);
PavlovFilters FromModel(const model::GameFilters& filters);

}  // namespace engine::game::pavlov
