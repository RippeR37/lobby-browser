#include "engine/games/contractors/contractors_data.h"

#include <set>

#include "base/logging.h"

namespace engine::game::contractors {

bool ContractorsGameModeFilters::AllEnabled() const {
  if (all) {
    return true;
  }

  bool any_disabled = false;
  bool any_enabled = false;

  dm ? (any_enabled = true, 0) : (any_disabled = true, 0);
  tdm ? (any_enabled = true, 0) : (any_disabled = true, 0);
  control ? (any_enabled = true, 0) : (any_disabled = true, 0);
  comp_control ? (any_enabled = true, 0) : (any_disabled = true, 0);
  bh_ffa ? (any_enabled = true, 0) : (any_disabled = true, 0);
  gungame_ffa ? (any_enabled = true, 0) : (any_disabled = true, 0);
  custom ? (any_enabled = true, 0) : (any_disabled = true, 0);
  domination ? (any_enabled = true, 0) : (any_disabled = true, 0);
  survival ? (any_enabled = true, 0) : (any_disabled = true, 0);
  rush ? (any_enabled = true, 0) : (any_disabled = true, 0);
  kc ? (any_enabled = true, 0) : (any_disabled = true, 0);
  zombie ? (any_enabled = true, 0) : (any_disabled = true, 0);
  other ? (any_enabled = true, 0) : (any_disabled = true, 0);

  return !any_disabled || !any_enabled;
}

std::vector<std::string> ContractorsGameModeFilters::ToVec() const {
  std::vector<std::string> enabled_game_modes;

  dm ? enabled_game_modes.emplace_back("DM"), 0 : 0;
  tdm ? enabled_game_modes.emplace_back("TDM"), 0 : 0;
  control ? enabled_game_modes.emplace_back("CONTROL"), 0 : 0;
  comp_control ? enabled_game_modes.emplace_back("COMP_CONTROL"), 0 : 0;
  bh_ffa ? enabled_game_modes.emplace_back("BH_FFA"), 0 : 0;
  gungame_ffa ? enabled_game_modes.emplace_back("GUNGAME_FFA"), 0 : 0;
  custom ? enabled_game_modes.emplace_back("CUSTOM"), 0 : 0;
  domination ? enabled_game_modes.emplace_back("DOMINATION"), 0 : 0;
  survival ? enabled_game_modes.emplace_back("SURVIVAL"), 0 : 0;
  rush ? enabled_game_modes.emplace_back("RUSH"), 0 : 0;
  kc ? enabled_game_modes.emplace_back("KC"), 0 : 0;
  zombie ? enabled_game_modes.emplace_back("ZOMBIE"), 0 : 0;
  other ? enabled_game_modes.emplace_back("OTHER"), 0 : 0;
  all ? enabled_game_modes.emplace_back("ALL"), 0 : 0;

  return enabled_game_modes;
}

bool ContractorsHostRegions::AllEnabled() const {
  return (america && europe && other) || (!america && !europe && !other);
}

std::vector<std::string> ContractorsHostRegions::ToVec() const {
  std::vector<std::string> enabled_regions;

  america ? (enabled_regions.emplace_back("America"), 0) : 0;
  europe ? (enabled_regions.emplace_back("Europe"), 0) : 0;
  other ? (enabled_regions.emplace_back("Other"), 0) : 0;

  return enabled_regions;
}

void to_json(nlohmann::json& out, const ContractorsGameModeFilters& obj) {
  out = obj.ToVec();
}

void from_json(const nlohmann::json& in, ContractorsGameModeFilters& obj) {
  std::vector<std::string> vec = in;
  std::set<std::string> enabled_game_modes{vec.begin(), vec.end()};

  obj.dm = enabled_game_modes.find("DM") != enabled_game_modes.end();
  obj.tdm = enabled_game_modes.find("TDM") != enabled_game_modes.end();
  obj.control = enabled_game_modes.find("CONTROL") != enabled_game_modes.end();
  obj.comp_control =
      enabled_game_modes.find("COMP_CONTROL") != enabled_game_modes.end();
  obj.bh_ffa = enabled_game_modes.find("BH_FFA") != enabled_game_modes.end();
  obj.gungame_ffa =
      enabled_game_modes.find("GUNGAME_FFA") != enabled_game_modes.end();
  obj.custom = enabled_game_modes.find("CUSTOM") != enabled_game_modes.end();
  obj.domination =
      enabled_game_modes.find("DOMINATION") != enabled_game_modes.end();
  obj.survival =
      enabled_game_modes.find("SURVIVAL") != enabled_game_modes.end();
  obj.rush = enabled_game_modes.find("RUSH") != enabled_game_modes.end();
  obj.kc = enabled_game_modes.find("KC") != enabled_game_modes.end();
  obj.zombie = enabled_game_modes.find("ZOMBIE") != enabled_game_modes.end();
  obj.other = enabled_game_modes.find("OTHER") != enabled_game_modes.end();
  obj.all = enabled_game_modes.find("ALL") != enabled_game_modes.end();
}

void to_json(nlohmann::json& out, const ContractorsHostRegions& obj) {
  out = nlohmann::json{
      {"america", obj.america},
      {"europe", obj.europe},
      {"other", obj.other},
  };
}

void from_json(const nlohmann::json& in, ContractorsHostRegions& obj) {
  obj.america = in.value("america", false);
  obj.europe = in.value("europe", false);
  obj.other = in.value("other", false);
}

void to_json(nlohmann::json& out, const ContractorsHostModeFilters& obj) {
  out = {
      {"casual", obj.casual},
      {"competitive", obj.competitive},
      {"other", obj.other},
      {"regions", obj.regions},
  };
}

void from_json(const nlohmann::json& in, ContractorsHostModeFilters& obj) {
  obj.casual = in.value("casual", false);
  obj.competitive = in.value("competitive", false);
  obj.other = in.value("other", false);
  obj.regions = in.value("regions", ContractorsHostRegions{});
}

void to_json(nlohmann::json& out, const ContractorsOtherFilters& obj) {
  out = {
      {"hide_full", obj.hide_full},
      {"hide_locked", obj.hide_locked},
  };
}

void from_json(const nlohmann::json& in, ContractorsOtherFilters& obj) {
  obj.hide_full = in.value("hide_full", false);
  obj.hide_locked = in.value("hide_locked", false);
}

void to_json(nlohmann::json& out, const ContractorsFilters& obj) {
  out = {
      {"game_modes", obj.game_modes},
      {"host_modes", obj.host_modes},
      {"others", obj.others},
  };
}

void from_json(const nlohmann::json& in, ContractorsFilters& obj) {
  obj.game_modes = in.value("game_modes", ContractorsGameModeFilters{});
  obj.host_modes = in.value("host_modes", ContractorsHostModeFilters{});
  obj.others = in.value("others", ContractorsOtherFilters{});
}

void to_json(nlohmann::json& out, const ContractorsConfig& obj) {
  out = {
      {"filters", obj.filters},
  };
}

void from_json(const nlohmann::json& in, ContractorsConfig& obj) {
  obj.filters = in.value("filters", ContractorsFilters{});
}

model::GameFilters ToModel(const ContractorsFilters& filters) {
  return {{
              "Game modes",
              {
                  {"Deathmatch", filters.game_modes.dm},
                  {"Team Deatchmatch", filters.game_modes.tdm},
                  {"Control", filters.game_modes.control},
                  {"Comp Control", filters.game_modes.comp_control},
                  {"BH FFA", filters.game_modes.bh_ffa},
                  {"Gun Game FFA", filters.game_modes.gungame_ffa},
                  {"Custom", filters.game_modes.custom},
                  {"Domination", filters.game_modes.domination},
                  {"Survival", filters.game_modes.survival},
                  {"Rush", filters.game_modes.rush},
                  {"KC", filters.game_modes.kc},
                  {"Zombie", filters.game_modes.zombie},
                  {"Other", filters.game_modes.other},
                  {"All", filters.game_modes.all},
              },
          },
          {
              "Host mode",
              {
                  {"Casual", filters.host_modes.casual},
                  {"America", filters.host_modes.regions.america},
                  {"Competitive", filters.host_modes.competitive},
                  {"Europe", filters.host_modes.regions.europe},
                  {"Other (GT)", filters.host_modes.other},
                  {"Other (LOC)", filters.host_modes.regions.other},
              },
          },
          {
              "Other",
              {
                  {"Hide Full", filters.others.hide_full, true},
                  {"Hide Locked", filters.others.hide_locked, true},
              },
          }};
}

ContractorsFilters FromModel(const model::GameFilters& filters) {
  ContractorsFilters result{};

  for (const auto& filter_group : filters) {
    std::map<std::string_view, bool> options;
    for (const auto& option : filter_group.filter_options) {
      options[option.name] = option.enabled;
    }

    auto value_or = [&options](std::string_view name, bool default_value) {
      auto option_it = options.find(name);
      if (option_it != options.end()) {
        return option_it->second;
      }
      return default_value;
    };

    if (filter_group.name == "Game modes") {
      result.game_modes.dm = value_or("Deathmatch", false);
      result.game_modes.tdm = value_or("Team Deatchmatch", false);
      result.game_modes.control = value_or("Control", false);
      result.game_modes.comp_control = value_or("Comp Control", false);
      result.game_modes.bh_ffa = value_or("BH FFA", false);
      result.game_modes.gungame_ffa = value_or("Gun Game FFA", false);
      result.game_modes.custom = value_or("Custom", false);
      result.game_modes.domination = value_or("Domination", false);
      result.game_modes.survival = value_or("Survival", false);
      result.game_modes.rush = value_or("Rush", false);
      result.game_modes.kc = value_or("KC", false);
      result.game_modes.zombie = value_or("Zombie", false);
      result.game_modes.other = value_or("Other", false);
      result.game_modes.all = value_or("All", false);
    } else if (filter_group.name == "Host mode") {
      result.host_modes.casual = value_or("Casual", false);
      result.host_modes.competitive = value_or("Competitive", false);
      result.host_modes.other = value_or("Other (GT)", false);
      result.host_modes.regions.america = value_or("America", false);
      result.host_modes.regions.europe = value_or("Europe", false);
      result.host_modes.regions.other = value_or("Other (LOC)", false);
    } else if (filter_group.name == "Other") {
      result.others.hide_full = value_or("Hide Full", false);
      result.others.hide_locked = value_or("Hide Locked", false);
    } else {
      LOG(ERROR) << __FUNCTION__
                 << "() unexpected filter group: " << filter_group.name;
    }
  }

  return result;
}

}  // namespace engine::game::contractors
