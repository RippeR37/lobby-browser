#include "engine/games/pavlov/pavlov_data.h"

#include <set>

#include "base/logging.h"

namespace engine::game::pavlov {

bool PavlovGameModeFilters::AllEnabled() const {
  if (all) {
    return true;
  }

  bool any_disabled = false;
  bool any_enabled = false;

  dm ? (any_enabled = true, 0) : (any_disabled = true, 0);
  tdm ? (any_enabled = true, 0) : (any_disabled = true, 0);
  snd ? (any_enabled = true, 0) : (any_disabled = true, 0);
  gun ? (any_enabled = true, 0) : (any_disabled = true, 0);
  ww2tdm ? (any_enabled = true, 0) : (any_disabled = true, 0);
  ww2gun ? (any_enabled = true, 0) : (any_disabled = true, 0);
  custom ? (any_enabled = true, 0) : (any_disabled = true, 0);
  ttt ? (any_enabled = true, 0) : (any_disabled = true, 0);
  oitc ? (any_enabled = true, 0) : (any_disabled = true, 0);
  hide ? (any_enabled = true, 0) : (any_disabled = true, 0);
  push ? (any_enabled = true, 0) : (any_disabled = true, 0);
  zombies ? (any_enabled = true, 0) : (any_disabled = true, 0);
  ph ? (any_enabled = true, 0) : (any_disabled = true, 0);
  infection ? (any_enabled = true, 0) : (any_disabled = true, 0);
  koth ? (any_enabled = true, 0) : (any_disabled = true, 0);

  return !any_disabled || !any_enabled;
}

std::vector<std::string> PavlovGameModeFilters::ToVec() const {
  std::vector<std::string> enabled_game_modes;

  dm ? enabled_game_modes.emplace_back("DM"), 0 : 0;
  tdm ? enabled_game_modes.emplace_back("TDM"), 0 : 0;
  snd ? enabled_game_modes.emplace_back("SND"), 0 : 0;
  gun ? enabled_game_modes.emplace_back("GUN"), 0 : 0;
  ww2tdm ? enabled_game_modes.emplace_back("WW2TDM"), 0 : 0;
  ww2gun ? enabled_game_modes.emplace_back("WW2GUN"), 0 : 0;
  custom ? enabled_game_modes.emplace_back("CUSTOM"), 0 : 0;
  ttt ? enabled_game_modes.emplace_back("TTT"), 0 : 0;
  oitc ? enabled_game_modes.emplace_back("OITC"), 0 : 0;
  hide ? enabled_game_modes.emplace_back("HIDE"), 0 : 0;
  push ? enabled_game_modes.emplace_back("PUSH"), 0 : 0;
  zombies ? enabled_game_modes.emplace_back("ZWV"), 0 : 0;
  ph ? enabled_game_modes.emplace_back("PH"), 0 : 0;
  infection ? enabled_game_modes.emplace_back("INFECTION"), 0 : 0;
  koth ? enabled_game_modes.emplace_back("KOTH"), 0 : 0;
  all ? enabled_game_modes.emplace_back("ALL"), 0 : 0;

  return enabled_game_modes;
}

bool PavlovHostRegions::AllEnabled() const {
  return (america && europe && asia_pacific) ||
         (!america && !europe && !asia_pacific);
}

std::vector<std::string> PavlovHostRegions::ToVec() const {
  std::vector<std::string> enabled_regions;

  america ? (enabled_regions.emplace_back("America"), 0) : 0;
  europe ? (enabled_regions.emplace_back("Europe"), 0) : 0;
  asia_pacific ? (enabled_regions.emplace_back("AsiaPacific"), 0) : 0;

  return enabled_regions;
}

void to_json(nlohmann::json& out, const PavlovGameModeFilters& obj) {
  out = obj.ToVec();
}

void from_json(const nlohmann::json& in, PavlovGameModeFilters& obj) {
  std::vector<std::string> vec = in;
  std::set<std::string> enabled_game_modes{vec.begin(), vec.end()};

  obj.dm = enabled_game_modes.find("DM") != enabled_game_modes.end();
  obj.tdm = enabled_game_modes.find("TDM") != enabled_game_modes.end();
  obj.snd = enabled_game_modes.find("SND") != enabled_game_modes.end();
  obj.gun = enabled_game_modes.find("GUN") != enabled_game_modes.end();
  obj.ww2tdm = enabled_game_modes.find("WW2TDM") != enabled_game_modes.end();
  obj.ww2gun = enabled_game_modes.find("WW2GUN") != enabled_game_modes.end();
  obj.custom = enabled_game_modes.find("CUSTOM") != enabled_game_modes.end();
  obj.ttt = enabled_game_modes.find("TTT") != enabled_game_modes.end();
  obj.oitc = enabled_game_modes.find("OITC") != enabled_game_modes.end();
  obj.hide = enabled_game_modes.find("HIDE") != enabled_game_modes.end();
  obj.push = enabled_game_modes.find("PUSH") != enabled_game_modes.end();
  obj.zombies = enabled_game_modes.find("ZWV") != enabled_game_modes.end();
  obj.ph = enabled_game_modes.find("PH") != enabled_game_modes.end();
  obj.infection =
      enabled_game_modes.find("INFECTION") != enabled_game_modes.end();
  obj.koth = enabled_game_modes.find("KOTH") != enabled_game_modes.end();
  obj.all = enabled_game_modes.find("ALL") != enabled_game_modes.end();
}

void to_json(nlohmann::json& out, const PavlovHostRegions& obj) {
  out = nlohmann::json{
      {"america", obj.america},
      {"europe", obj.europe},
      {"asia_pacific", obj.asia_pacific},
  };
}

void from_json(const nlohmann::json& in, PavlovHostRegions& obj) {
  obj.america = in.value("america", false);
  obj.europe = in.value("europe", false);
  obj.asia_pacific = in.value("asia_pacific", false);
}

void to_json(nlohmann::json& out, const PavlovHostModeFilters& obj) {
  out = {
      {"lobby", obj.lobby},
      {"server", obj.server},
      {"crossplay", obj.crossplay},
      {"regions", obj.regions},
  };
}

void from_json(const nlohmann::json& in, PavlovHostModeFilters& obj) {
  obj.lobby = in.value("lobby", false);
  obj.server = in.value("server", false);
  obj.crossplay = in.value("crossplay", false);
  obj.regions = in.value("regions", PavlovHostRegions{});
}

void to_json(nlohmann::json& out, const PavlovOtherFilters& obj) {
  out = {
      {"hide_full", obj.hide_full},
      {"hide_empty", obj.hide_empty},
      {"hide_locked", obj.hide_locked},
      {"only_compatible", obj.only_compatible},
  };
}

void from_json(const nlohmann::json& in, PavlovOtherFilters& obj) {
  obj.hide_full = in.value("hide_full", false);
  obj.hide_empty = in.value("hide_empty", false);
  obj.hide_locked = in.value("hide_locked", false);
  obj.only_compatible = in.value("only_compatible", false);
}

void to_json(nlohmann::json& out, const PavlovFilters& obj) {
  out = {
      {"game_modes", obj.game_modes},
      {"host_modes", obj.host_modes},
      {"others", obj.others},
  };
}

void from_json(const nlohmann::json& in, PavlovFilters& obj) {
  obj.game_modes = in.value("game_modes", PavlovGameModeFilters{});
  obj.host_modes = in.value("host_modes", PavlovHostModeFilters{});
  obj.others = in.value("others", PavlovOtherFilters{});
}

void to_json(nlohmann::json& out, const PavlovConfig& obj) {
  out = {
      {"game_version", obj.game_version},
      {"filters", obj.filters},
  };
}

void from_json(const nlohmann::json& in, PavlovConfig& obj) {
  obj.game_version = in.value("game_version", "");
  obj.filters = in.value("filters", PavlovFilters{});
}

void from_json(const nlohmann::json& in, PavlovServer& obj) {
  obj.name = in.value("name", "");
  obj.slots = in.value("slots", -1);
  obj.max_slots = in.value("maxSlots", -1);
  obj.map_id = in.value("mapId", "");
  obj.map_label = in.value("mapLabel", "");
  obj.port = in.value("port", -1);
  obj.password_protected = in.value("bPasswordProtected", false);
  obj.secured = in.value("bSecured", true);
  obj.game_mode = in.value("gameMode", "");
  obj.game_mode_label = in.value("gameModeLabel", "");
  obj.ip = in.value("ip", "");
  obj.version = in.value("version", "");
  obj.updated_ts = in.value("updated", "");
}

void from_json(const nlohmann::json& in, PavlovServerListResponse& obj) {
  obj.result = in.value("result", "Success");
  obj.servers = in.value("servers", std::vector<PavlovServer>{});
}

model::GameFilters ToModel(const PavlovFilters& filters) {
  return {{
              "Game modes",
              {
                  {"Deathmatch", filters.game_modes.dm},
                  {"Team Deatchmatch", filters.game_modes.tdm},
                  {"Search && Destroy", filters.game_modes.snd},
                  {"Gun Game", filters.game_modes.gun},
                  {"WW2 TDM", filters.game_modes.ww2tdm},
                  {"WW2 Gun Game", filters.game_modes.ww2gun},
                  {"CUSTOM", filters.game_modes.custom},
                  {"TTTv2", filters.game_modes.ttt},
                  {"One In The Chamber", filters.game_modes.oitc},
                  {"The Hide", filters.game_modes.hide},
                  {"Push", filters.game_modes.push},
                  {"Zombies", filters.game_modes.zombies},
                  {"Prop Hunt", filters.game_modes.ph},
                  {"Infection", filters.game_modes.infection},
                  {"King Of The Hill", filters.game_modes.koth},
                  {"All", filters.game_modes.all},
              },
          },
          {
              "Host mode",
              {
                  {"Lobby", filters.host_modes.lobby},
                  {"America", filters.host_modes.regions.america},
                  {"Server", filters.host_modes.server},
                  {"Europe", filters.host_modes.regions.europe},
                  {"Crossplay", filters.host_modes.crossplay},
                  {"Asia/Pacific", filters.host_modes.regions.asia_pacific},
              },
          },
          {
              "Other",
              {
                  {"Hide Full", filters.others.hide_full, true},
                  {"Hide Empty", filters.others.hide_empty, true},
                  {"Hide Locked", filters.others.hide_locked, true},
                  {"Only Compatible", filters.others.only_compatible, true},
              },
          }};
}

PavlovFilters FromModel(const model::GameFilters& filters) {
  PavlovFilters result{};

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
      result.game_modes.snd = value_or("Search && Destroy", false);
      result.game_modes.gun = value_or("Gun Game", false);
      result.game_modes.ww2tdm = value_or("WW2 TDM", false);
      result.game_modes.ww2gun = value_or("WW2 Gun Game", false);
      result.game_modes.custom = value_or("CUSTOM", false);
      result.game_modes.ttt = value_or("TTTv2", false);
      result.game_modes.oitc = value_or("One In The Chamber", false);
      result.game_modes.hide = value_or("The Hide", false);
      result.game_modes.push = value_or("Push", false);
      result.game_modes.zombies = value_or("Zombies", false);
      result.game_modes.ph = value_or("Prop Hunt", false);
      result.game_modes.infection = value_or("Infection", false);
      result.game_modes.koth = value_or("King Of The Hill", false);
      result.game_modes.all = value_or("All", false);
    } else if (filter_group.name == "Host mode") {
      result.host_modes.lobby = value_or("Lobby", false);
      result.host_modes.server = value_or("Server", false);
      result.host_modes.crossplay = value_or("Crossplay", false);
      result.host_modes.regions.america = value_or("America", false);
      result.host_modes.regions.europe = value_or("Europe", false);
      result.host_modes.regions.asia_pacific = value_or("Asia/Pacific", false);
    } else if (filter_group.name == "Other") {
      result.others.hide_full = value_or("Hide Full", false);
      result.others.hide_empty = value_or("Hide Empty", false);
      result.others.hide_locked = value_or("Hide Locked", false);
      result.others.only_compatible = value_or("Only Compatible", false);
    } else {
      LOG(ERROR) << __FUNCTION__
                 << "() unexpected filter group: " << filter_group.name;
    }
  }

  return result;
}

}  // namespace engine::game::pavlov
