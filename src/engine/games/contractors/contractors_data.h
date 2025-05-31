#pragma once

#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "models/game.h"

namespace engine::game::contractors {

struct ContractorsGameModeFilters {
  bool dm;
  bool tdm;
  bool control;
  bool comp_control;
  bool bh_ffa;
  bool gungame_ffa;
  bool custom;
  bool domination;
  bool survival;
  bool rush;
  bool kc;
  bool zombie;
  bool other;
  bool all;

  bool AllEnabled() const;
  std::vector<std::string> ToVec() const;
};

struct ContractorsHostRegions {
  bool america;
  bool europe;
  bool other;

  bool AllEnabled() const;
  std::vector<std::string> ToVec() const;
};

struct ContractorsHostModeFilters {
  bool casual;
  bool competitive;
  bool other;

  ContractorsHostRegions regions;
};

struct ContractorsOtherFilters {
  bool hide_full;
  bool hide_locked;
};

struct ContractorsFilters {
  ContractorsGameModeFilters game_modes;
  ContractorsHostModeFilters host_modes;
  ContractorsOtherFilters others;
};

struct ContractorsConfig {
  ContractorsFilters filters;
};

//
// JSON
//

void to_json(nlohmann::json& out, const ContractorsGameModeFilters& obj);
void from_json(const nlohmann::json& in, ContractorsGameModeFilters& obj);

void to_json(nlohmann::json& out, const ContractorsHostRegions& obj);
void from_json(const nlohmann::json& in, ContractorsHostRegions& obj);

void to_json(nlohmann::json& out, const ContractorsHostModeFilters& obj);
void from_json(const nlohmann::json& in, ContractorsHostModeFilters& obj);

void to_json(nlohmann::json& out, const ContractorsOtherFilters& obj);
void from_json(const nlohmann::json& in, ContractorsOtherFilters& obj);

void to_json(nlohmann::json& out, const ContractorsFilters& obj);
void from_json(const nlohmann::json& in, ContractorsFilters& obj);

void to_json(nlohmann::json& out, const ContractorsConfig& obj);
void from_json(const nlohmann::json& in, ContractorsConfig& obj);

//
// Game model
//

model::GameFilters ToModel(const ContractorsFilters& filters);
ContractorsFilters FromModel(const model::GameFilters& filters);

}  // namespace engine::game::contractors
