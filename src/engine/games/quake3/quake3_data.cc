#include "engine/games/quake3/quake3_data.h"

namespace engine::game::quake3 {

void to_json(nlohmann::json& out, const Quake3MasterServer& obj) {
  out = nlohmann::json{
      {"name", obj.name},
      {"address", obj.address},
      {"built_in", obj.built_in},
      {"enabled", obj.enabled},
  };
}

void from_json(const nlohmann::json& in, Quake3MasterServer& obj) {
  obj.name = in.value("name", std::string{});
  obj.address = in.value("address", std::string{});
  obj.built_in = in.value("built_in", false);
  obj.enabled = in.value("enabled", true);
}

void to_json(nlohmann::json& out, const Quake3OtherFilters& obj) {
  out = nlohmann::json{
      {"hide_full", obj.hide_full},
      {"hide_empty", obj.hide_empty},
  };
}

void from_json(const nlohmann::json& in, Quake3OtherFilters& obj) {
  obj.hide_full = in.value("hide_full", false);
  obj.hide_empty = in.value("hide_empty", false);
}

void to_json(nlohmann::json& out, const Quake3GameModeFilters& obj) {
  out = nlohmann::json{
      {"all", obj.all},
      {"ffa", obj.ffa},
      {"tournament", obj.tournament},
      {"single", obj.single},
      {"tdm", obj.tdm},
      {"ctf", obj.ctf},
      {"one_flag_ctf", obj.one_flag_ctf},
      {"overload", obj.overload},
      {"harvester", obj.harvester},
      {"team_ffa", obj.team_ffa},
  };
}

void from_json(const nlohmann::json& in, Quake3GameModeFilters& obj) {
  obj.all = in.value("all", true);
  obj.ffa = in.value("ffa", true);
  obj.tournament = in.value("tournament", true);
  obj.single = in.value("single", true);
  obj.tdm = in.value("tdm", true);
  obj.ctf = in.value("ctf", true);
  obj.one_flag_ctf = in.value("one_flag_ctf", true);
  obj.overload = in.value("overload", true);
  obj.harvester = in.value("harvester", true);
  obj.team_ffa = in.value("team_ffa", true);
}

void to_json(nlohmann::json& out, const Quake3PingFilters& obj) {
  out = nlohmann::json{
      {"all", obj.all},
      {"max_50", obj.max_50},
      {"max_100", obj.max_100},
      {"max_200", obj.max_200},
  };
}

void from_json(const nlohmann::json& in, Quake3PingFilters& obj) {
  obj.all = in.value("all", true);
  obj.max_50 = in.value("max_50", false);
  obj.max_100 = in.value("max_100", false);
  obj.max_200 = in.value("max_200", false);
}

void to_json(nlohmann::json& out, const Quake3Filters& obj) {
  out = nlohmann::json{
      {"master_servers", obj.master_servers},
      {"game_modes", obj.game_modes},
      {"pings", obj.pings},
      {"others", obj.others},
  };
}

void from_json(const nlohmann::json& in, Quake3Filters& obj) {
  obj.master_servers =
      in.value("master_servers", std::vector<Quake3MasterServer>{});
  obj.game_modes = in.value("game_modes", Quake3GameModeFilters{});
  obj.pings = in.value("pings", Quake3PingFilters{});
  obj.others = in.value("others", Quake3OtherFilters{});
}

void to_json(nlohmann::json& out, const Quake3Config& obj) {
  out = nlohmann::json{
      {"filters", obj.filters},
      {"executable_path", obj.executable_path},
  };
}

void from_json(const nlohmann::json& in, Quake3Config& obj) {
  obj.filters = in.value("filters", Quake3Filters{});
  obj.executable_path = in.value("executable_path", "");
}

model::GameFilters ToModel(const Quake3Filters& filters) {
  model::GameFilters model_filters;

  model::GameFilterGroup master_group{"Master Servers", {}};
  for (const auto& master : filters.master_servers) {
    master_group.filter_options.push_back(
        model::GameFilterOption{master.name, master.enabled});
  }
  model_filters.push_back(std::move(master_group));

  model::GameFilterGroup modes_group{"Game Modes", {}};
  modes_group.filter_options.push_back(
      model::GameFilterOption{"FFA", filters.game_modes.ffa, true});
  modes_group.filter_options.push_back(
      model::GameFilterOption{"1v1", filters.game_modes.tournament, true});
  modes_group.filter_options.push_back(
      model::GameFilterOption{"Single", filters.game_modes.single, true});
  modes_group.filter_options.push_back(
      model::GameFilterOption{"TDM", filters.game_modes.tdm, true});
  modes_group.filter_options.push_back(
      model::GameFilterOption{"CTF", filters.game_modes.ctf, true});
  modes_group.filter_options.push_back(model::GameFilterOption{
      "OneFlag CTF", filters.game_modes.one_flag_ctf, true});
  modes_group.filter_options.push_back(
      model::GameFilterOption{"Overload", filters.game_modes.overload, true});
  modes_group.filter_options.push_back(
      model::GameFilterOption{"Harvester", filters.game_modes.harvester, true});
  modes_group.filter_options.push_back(
      model::GameFilterOption{"Team FFA", filters.game_modes.team_ffa, true});
  modes_group.filter_options.push_back(
      model::GameFilterOption{"All", filters.game_modes.all, true, true});
  model_filters.push_back(std::move(modes_group));

  model::GameFilterGroup pings_group{
      "Max Ping", {}, model::GameFilterGroupType::kRadioButton};
  pings_group.filter_options.push_back(
      model::GameFilterOption{"<50", filters.pings.max_50, true});
  pings_group.filter_options.push_back(
      model::GameFilterOption{"<100", filters.pings.max_100, true});
  pings_group.filter_options.push_back(
      model::GameFilterOption{"<200", filters.pings.max_200, true});
  pings_group.filter_options.push_back(
      model::GameFilterOption{"All", filters.pings.all, true});
  model_filters.push_back(std::move(pings_group));

  model::GameFilterGroup other_group{"Other", {}};
  other_group.filter_options.push_back(
      model::GameFilterOption{"Hide Full", filters.others.hide_full, true});
  other_group.filter_options.push_back(
      model::GameFilterOption{"Hide Empty", filters.others.hide_empty, true});
  model_filters.push_back(std::move(other_group));

  return model_filters;
}

Quake3Filters FromModel(const model::GameFilters& model_filters) {
  Quake3Filters filters;

  for (const auto& group : model_filters) {
    if (group.name == "Master Servers") {
      for (const auto& option : group.filter_options) {
        filters.master_servers.push_back(
            Quake3MasterServer{option.name, "", false, option.enabled});
      }
    } else if (group.name == "Game Modes") {
      for (const auto& option : group.filter_options) {
        if (option.name == "All")
          filters.game_modes.all = option.enabled;
        else if (option.name == "FFA")
          filters.game_modes.ffa = option.enabled;
        else if (option.name == "1v1")
          filters.game_modes.tournament = option.enabled;
        else if (option.name == "Single")
          filters.game_modes.single = option.enabled;
        else if (option.name == "TDM")
          filters.game_modes.tdm = option.enabled;
        else if (option.name == "CTF")
          filters.game_modes.ctf = option.enabled;
        else if (option.name == "OneFlag CTF")
          filters.game_modes.one_flag_ctf = option.enabled;
        else if (option.name == "Overload")
          filters.game_modes.overload = option.enabled;
        else if (option.name == "Harvester")
          filters.game_modes.harvester = option.enabled;
        else if (option.name == "Team FFA")
          filters.game_modes.team_ffa = option.enabled;
      }
    } else if (group.name == "Max Ping") {
      for (const auto& option : group.filter_options) {
        if (option.name == "All")
          filters.pings.all = option.enabled;
        else if (option.name == "<50")
          filters.pings.max_50 = option.enabled;
        else if (option.name == "<100")
          filters.pings.max_100 = option.enabled;
        else if (option.name == "<200")
          filters.pings.max_200 = option.enabled;
      }
    } else if (group.name == "Other") {
      for (const auto& option : group.filter_options) {
        if (option.name == "Hide Full") {
          filters.others.hide_full = option.enabled;
        } else if (option.name == "Hide Empty") {
          filters.others.hide_empty = option.enabled;
        }
      }
    }
  }

  return filters;
}

}  // namespace engine::game::quake3
