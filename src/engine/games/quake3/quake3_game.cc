#include "engine/games/quake3/quake3_game.h"

#include <algorithm>
#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/bind_post_task.h"
#include "base/logging.h"

#include <windows.h>

// These need to be included *after* windows.h
#include <shellapi.h>

namespace engine::game {

namespace {

struct SearchRequestState {
  size_t total_servers = 0;
  std::atomic<size_t> queried_servers{0};
};

model::GameSearchResults FilterModelResultsFunction(
    const model::GameSearchResults& unfiltered_results,
    const model::GameFilters& model_filters) {
  bool hide_full = false;
  bool hide_empty = false;
  bool all_modes = false;
  std::set<std::string> enabled_modes;

  bool all_pings = true;
  int max_ping = 9999;

  for (const auto& group : model_filters) {
    if (group.name == "Other") {
      for (const auto& option : group.filter_options) {
        if (option.name == "Hide Full") {
          hide_full = option.enabled;
        } else if (option.name == "Hide Empty") {
          hide_empty = option.enabled;
        }
      }
    } else if (group.name == "Game Modes") {
      for (const auto& option : group.filter_options) {
        if (option.name == "All") {
          all_modes = option.enabled;
        }
        if (option.enabled) {
          enabled_modes.insert(option.name);
        }
      }
    } else if (group.name == "Max Ping") {
      for (const auto& option : group.filter_options) {
        if (option.name == "All") {
          all_pings = option.enabled;
        } else if (option.enabled) {
          if (option.name == "<50")
            max_ping = (std::min)(max_ping, 50);
          else if (option.name == "<100")
            max_ping = (std::min)(max_ping, 100);
          else if (option.name == "<200")
            max_ping = (std::min)(max_ping, 200);
        }
      }
    }
  }

  model::GameSearchResults filtered_results;
  for (const auto& result : unfiltered_results.lobbies) {
    // result_fields: Address (0), Game Mode (1), Hostname (2), Map (3), Players
    // (4), Humans (5), Ping (6)
    if (result.result_fields.size() >= 7) {
      // 1. Game Mode Filter
      if (!all_modes) {
        const std::string& mode = result.result_fields[1];
        if (enabled_modes.find(mode) == enabled_modes.end()) {
          continue;
        }
      }

      // 2. Ping Filter
      if (!all_pings) {
        try {
          int ping = std::stoi(result.result_fields[6]);
          if (ping > max_ping)
            continue;
        } catch (...) {
        }
      }

      // 3. Players Filter
      const std::string& players_str = result.result_fields[4];
      size_t slash_pos = players_str.find('/');
      if (slash_pos != std::string::npos) {
        try {
          int current = std::stoi(players_str.substr(0, slash_pos));
          int max = std::stoi(players_str.substr(slash_pos + 1));

          if (hide_empty && current == 0)
            continue;
          if (hide_full && current >= max && max > 0)
            continue;
        } catch (...) {
          // If parsing fails, don't filter this entry
        }
      }
    }
    filtered_results.lobbies.emplace_back(result);
  }
  return filtered_results;
}

struct GroupKey {
  std::string hostname;
  std::string map;
  std::string game_type;
  int players;
  int max_players;

  bool operator<(const GroupKey& other) const {
    return std::tie(hostname, map, game_type, players, max_players) <
           std::tie(other.hostname, other.map, other.game_type, other.players,
                    other.max_players);
  }
};

bool AreMembersSame(const std::vector<backend::Quake3ServerResult::Member>& a,
                    const std::vector<backend::Quake3ServerResult::Member>& b) {
  if (a.size() != b.size())
    return false;
  if (a.empty())
    return true;

  auto get_canonical =
      [](const std::vector<backend::Quake3ServerResult::Member>& members) {
        std::vector<std::string> res;
        res.reserve(members.size());
        for (const auto& m : members) {
          res.push_back(m.name);
        }
        std::sort(res.begin(), res.end());
        return res;
      };

  return get_canonical(a) == get_canonical(b);
}

int GetPortFromAddress(const std::string& address) {
  size_t colon_pos = address.find_last_of(':');
  if (colon_pos == std::string::npos)
    return 0;
  try {
    return std::stoi(address.substr(colon_pos + 1));
  } catch (...) {
    return 0;
  }
}

}  // namespace

Quake3Game::Quake3Game(SetStatusTextCallback set_status_text,
                       ReportMessageCallback report_message,
                       nlohmann::json game_config)
    : BaseGame(std::move(set_status_text), std::move(report_message)),
      config_(),
      master_backend_(nullptr),
      weak_factory_(this) {
  weak_this_ = weak_factory_.GetWeakPtr();

  // Built-in master servers
  config_.filters.master_servers = {
      quake3::Quake3MasterServer{"id Software", "master.quake3arena.com:27950",
                                 true, true},
      quake3::Quake3MasterServer{"ioquake3", "directory.ioquake3.org:27950",
                                 true, true},
      quake3::Quake3MasterServer{"VR Servers", "mp.quakevr.com:27950", true,
                                 true},
  };

  if (!game_config.is_null()) {
    try {
      quake3::Quake3Config loaded_config = game_config;
      // Merge loaded config with built-ins, keeping order and built-in status
      for (const auto& loaded_master : loaded_config.filters.master_servers) {
        bool found = false;
        for (auto& built_in_master : config_.filters.master_servers) {
          if (built_in_master.address == loaded_master.address) {
            built_in_master.enabled = loaded_master.enabled;
            found = true;
            break;
          }
        }
        if (!found && !loaded_master.built_in) {
          config_.filters.master_servers.push_back(loaded_master);
        }
      }
      config_.filters.others = loaded_config.filters.others;
      config_.filters.game_modes = loaded_config.filters.game_modes;
      config_.filters.pings = loaded_config.filters.pings;
      config_.executable_path = std::move(loaded_config.executable_path);
    } catch (const std::exception& e) {
      LOG(ERROR) << __FUNCTION__
                 << "() failed to load game config: " << e.what();
    }
  }

  master_backend_ = backend::Quake3MasterBackend::Create();
}

Quake3Game::~Quake3Game() = default;

model::Game Quake3Game::GetModel() const {
  return model::Game{
      "Quake 3",
      "IDI_ICON_QUAKE3",
      model::GameFeatures{
          .has_lobby_details = true,
          .search_players = false,
          .list_players = true,
          .connect_to_lobby = false,
      },
      ToModel(config_.filters),
      model::GameResultsFormat{
          {
              model::GameResultsColumnFormat{
                  "Address",
                  false,
                  0,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Game Mode",
                  true,
                  120,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Hostname",
                  true,
                  300,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
                  true,
              },
              model::GameResultsColumnFormat{
                  "Map",
                  true,
                  120,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
                  true,
              },
              model::GameResultsColumnFormat{
                  "Players",
                  true,
                  65,
                  model::GameResultsColumnAlignment::kRight,
                  model::GameResultsColumnOrdering::kNumberDivNumber,
                  true,
              },
              model::GameResultsColumnFormat{
                  "Humans",
                  true,
                  65,
                  model::GameResultsColumnAlignment::kRight,
                  model::GameResultsColumnOrdering::kNumberDivNumber,
                  true,
              },
              model::GameResultsColumnFormat{
                  "Ping",
                  true,
                  50,
                  model::GameResultsColumnAlignment::kRight,
                  model::GameResultsColumnOrdering::kNumber,
              },
          },
          {
              model::GameResultsColumnFormat{
                  "Lobby Id",
                  false,
                  0,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Player Id",
                  false,
                  0,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "",
                  false,
                  0,
                  model::GameResultsColumnAlignment::kCenter,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Score",
                  true,
                  60,
                  model::GameResultsColumnAlignment::kRight,
                  model::GameResultsColumnOrdering::kNumber,
              },
              model::GameResultsColumnFormat{
                  "Player Name",
                  true,
                  200,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Ping",
                  true,
                  50,
                  model::GameResultsColumnAlignment::kRight,
                  model::GameResultsColumnOrdering::kNumber,
              },
              model::GameResultsColumnFormat{
                  "Hostname",
                  true,
                  180,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Mode",
                  true,
                  100,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Map",
                  true,
                  100,
                  model::GameResultsColumnAlignment::kLeft,
                  model::GameResultsColumnOrdering::kString,
              },
              model::GameResultsColumnFormat{
                  "Players",
                  true,
                  60,
                  model::GameResultsColumnAlignment::kRight,
                  model::GameResultsColumnOrdering::kNumberDivNumber,
              },
              model::GameResultsColumnFormat{
                  "Humans",
                  true,
                  60,
                  model::GameResultsColumnAlignment::kRight,
                  model::GameResultsColumnOrdering::kNumberDivNumber,
              },
          },
      },
      base::BindRepeating(&FilterModelResultsFunction),
  };
}

nlohmann::json Quake3Game::UpdateConfig(model::GameFilters search_filters) {
  for (const auto& group : search_filters) {
    if (group.name == "Master Servers") {
      for (const auto& option : group.filter_options) {
        for (auto& master : config_.filters.master_servers) {
          if (master.name == option.name) {
            master.enabled = option.enabled;
            break;
          }
        }
      }
    } else if (group.name == "Game Modes") {
      for (const auto& option : group.filter_options) {
        if (option.name == "All")
          config_.filters.game_modes.all = option.enabled;
        else if (option.name == "FFA")
          config_.filters.game_modes.ffa = option.enabled;
        else if (option.name == "1v1")
          config_.filters.game_modes.tournament = option.enabled;
        else if (option.name == "Single")
          config_.filters.game_modes.single = option.enabled;
        else if (option.name == "TDM")
          config_.filters.game_modes.tdm = option.enabled;
        else if (option.name == "CTF")
          config_.filters.game_modes.ctf = option.enabled;
        else if (option.name == "OneFlag CTF")
          config_.filters.game_modes.one_flag_ctf = option.enabled;
        else if (option.name == "Overload")
          config_.filters.game_modes.overload = option.enabled;
        else if (option.name == "Harvester")
          config_.filters.game_modes.harvester = option.enabled;
        else if (option.name == "Team FFA")
          config_.filters.game_modes.team_ffa = option.enabled;
      }
    } else if (group.name == "Max Ping") {
      for (const auto& option : group.filter_options) {
        if (option.name == "All")
          config_.filters.pings.all = option.enabled;
        else if (option.name == "<50")
          config_.filters.pings.max_50 = option.enabled;
        else if (option.name == "<100")
          config_.filters.pings.max_100 = option.enabled;
        else if (option.name == "<200")
          config_.filters.pings.max_200 = option.enabled;
      }
    } else if (group.name == "Other") {
      for (const auto& option : group.filter_options) {
        if (option.name == "Hide Full") {
          config_.filters.others.hide_full = option.enabled;
        } else if (option.name == "Hide Empty") {
          config_.filters.others.hide_empty = option.enabled;
        }
      }
    }
  }
  return config_;
}

void Quake3Game::SearchLobbiesAndServers(
    model::SearchRequest request,
    base::OnceCallback<void(model::SearchResponse)> on_done_callback) {
  // Update internal config with filters from the UI request before searching.
  UpdateConfig(std::move(request.search_filters));

  if (!master_backend_) {
    std::move(on_done_callback)
        .Run(model::SearchResponse{model::SearchResult::kUnsupported,
                                   "Backend not implemented",
                                   {},
                                   {}});
    return;
  }

  SetStatusText("Searching master servers...");

  std::vector<std::string> enabled_masters;
  for (const auto& master : config_.filters.master_servers) {
    if (master.enabled) {
      enabled_masters.push_back(master.address);
    }
  }

  if (enabled_masters.empty()) {
    std::move(on_done_callback)
        .Run(model::SearchResponse{model::SearchResult::kOk, "", {}, {}});
    return;
  }

  master_backend_->SearchServers(
      enabled_masters,
      base::BindToCurrentSequence(
          base::BindOnce(&Quake3Game::OnMasterSearchDone, weak_this_,
                         std::move(on_done_callback),
                         std::move(request.players_callback)),
          FROM_HERE));
}

void Quake3Game::GetServerLobbyDetails(
    model::SearchDetailsRequest request,
    base::OnceCallback<void(model::SearchDetailsResponse)> on_done_callback) {
  if (!last_response_results_) {
    std::move(on_done_callback).Run({});
    return;
  }

  for (const auto& server : *last_response_results_) {
    if (server.address == request.result_id) {
      model::SearchDetailsResponse response{true, {}};
      for (const auto& member : server.members) {
        model::SearchDetailsResponse::Member m;
        m.id = member.name;
        m.name = member.name;
        m.avatar_url = "";  // Generic placeholder used in UI
        m.user_data.push_back({"Score", std::to_string(member.score)});
        m.user_data.push_back({"Ping", std::to_string(member.ping)});
        m.user_data.push_back({"IsBot", (member.ping == 0 ? "true" : "false")});
        response.members.push_back(std::move(m));
      }
      std::move(on_done_callback).Run(std::move(response));
      return;
    }
  }

  std::move(on_done_callback).Run({});
}

void Quake3Game::SearchUsers(
    model::SearchUsersRequest request,
    base::OnceCallback<void(model::SearchUsersResponse)> on_done_callback) {
  (void)request;
  std::move(on_done_callback)
      .Run({model::SearchResult::kUnsupported, "Not supported", {}});
}

model::GameConfigDescriptor Quake3Game::GetConfigDescriptor() const {
  model::GameConfigDescriptor descriptor;

  // General tab
  model::GameConfigSection general;
  general.name = "General";
  general.options.push_back({
      "executable_path",
      "Quake 3 executable path",
      "Select the Quake 3 executable for 'Start game && connect' feature",
      model::GameConfigOptionType::kFilePath,
      config_.executable_path,
      {},  // list_columns
      {},  // list_items
  });
  descriptor.sections.push_back(std::move(general));

  // Master servers tab
  model::GameConfigSection master_servers;
  master_servers.name = "Master Servers";
  model::GameConfigOption masters;
  masters.key = "master_servers";
  masters.label = "Custom master servers";
  masters.description = "Add or remove custom master servers for Quake 3";
  masters.type = model::GameConfigOptionType::kEditableList;
  masters.list_columns = {{"Name", 200}, {"Address", 250}};
  for (const auto& master : config_.filters.master_servers) {
    if (!master.built_in) {
      masters.list_items.push_back(
          {master.address, {master.name, master.address}});
    }
  }
  master_servers.options.push_back(std::move(masters));
  descriptor.sections.push_back(std::move(master_servers));

  return descriptor;
}

void Quake3Game::UpdateConfigOption(std::string key, std::string value) {
  if (key == "executable_path") {
    config_.executable_path = std::move(value);
  }
}

void Quake3Game::AddConfigListItem(std::string key,
                                   std::vector<std::string> fields) {
  if (key == "master_servers" && fields.size() >= 2) {
    const std::string& name = fields[0];
    const std::string& address = fields[1];

    for (const auto& master : config_.filters.master_servers) {
      if (master.address == address)
        return;
    }
    config_.filters.master_servers.push_back(
        quake3::Quake3MasterServer{name, address, false, true});
  }
}

void Quake3Game::RemoveConfigListItem(std::string key, std::string item_id) {
  if (key == "master_servers") {
    for (auto it = config_.filters.master_servers.begin();
         it != config_.filters.master_servers.end(); ++it) {
      if (it->address == item_id && !it->built_in) {
        config_.filters.master_servers.erase(it);
        break;
      }
    }
  }
}

bool Quake3Game::CommitConfig(model::GameConfigDescriptor descriptor) {
  bool needs_reinit = false;

  for (const auto& section : descriptor.sections) {
    for (const auto& option : section.options) {
      if (option.key == "executable_path") {
        config_.executable_path = option.value;
      } else if (option.key == "master_servers") {
        // Clear non-built-in and rebuild from items
        auto old_servers = config_.filters.master_servers;

        auto it = std::remove_if(config_.filters.master_servers.begin(),
                                 config_.filters.master_servers.end(),
                                 [](const auto& m) { return !m.built_in; });
        config_.filters.master_servers.erase(
            it, config_.filters.master_servers.end());

        for (const auto& item : option.list_items) {
          if (item.fields.size() >= 2) {
            config_.filters.master_servers.push_back(quake3::Quake3MasterServer{
                item.fields[0], item.fields[1], false, true});
          }
        }

        if (!needs_reinit) {
          auto new_servers = config_.filters.master_servers;

          if (old_servers.size() != new_servers.size()) {
            needs_reinit = true;
          } else {
            auto cmp_server = [](const quake3::Quake3MasterServer& a,
                                 const quake3::Quake3MasterServer& b) {
              return a.address < b.address;
            };

            std::sort(old_servers.begin(), old_servers.end(), cmp_server);
            std::sort(new_servers.begin(), new_servers.end(), cmp_server);

            for (size_t idx = 0; idx < old_servers.size(); ++idx) {
              if (std::tie(old_servers[idx].address, old_servers[idx].name) !=
                  std::tie(new_servers[idx].address, new_servers[idx].name)) {
                needs_reinit = true;
                break;
              }
            }
          }
        }
      }
    }
  }

  return needs_reinit;
}

void Quake3Game::LaunchGame(std::string server_address) {
  if (config_.executable_path.empty()) {
    ReportMessage(
        Presenter::MessageType::kError, "Quake 3 executable path not set",
        "Please configure Quake 3 settings before using this feature.\n"
        "You can do this in Application -> Settings menu.");
    return;
  }

  std::string params = "+connect " + server_address;

  // Working directory
  std::string working_dir;
  size_t last_slash = config_.executable_path.find_last_of("\\/");
  if (last_slash != std::string::npos) {
    working_dir = config_.executable_path.substr(0, last_slash);
  }

  HINSTANCE result =
      ShellExecuteA(nullptr, "open", config_.executable_path.c_str(),
                    params.c_str(), working_dir.c_str(), SW_SHOW);

  if (reinterpret_cast<uintptr_t>(result) <= 32) {
    ReportMessage(
        Presenter::MessageType::kError, "Failed to launch Quake 3",
        "Error code: " + std::to_string(reinterpret_cast<uintptr_t>(result)));
  }
}

void Quake3Game::OnMasterSearchDone(
    base::OnceCallback<void(model::SearchResponse)> on_done_callback,
    base::OnceCallback<void(model::GamePlayersResults)> players_callback,
    backend::Quake3MasterSearchResponse response) {
  if (response.result.status != backend::Result::Status::kOk) {
    std::move(on_done_callback)
        .Run(model::SearchResponse{
            model::SearchResult::kError, response.result.error, {}, {}});
    return;
  }

  std::map<GroupKey, std::vector<backend::Quake3ServerResult>> grouped_servers;

  for (auto& server : response.servers) {
    GroupKey key{server.hostname, server.map, server.game_type, server.players,
                 server.max_players};
    auto& group = grouped_servers[key];

    bool is_duplicate = false;
    for (auto& existing : group) {
      if (AreMembersSame(server.members, existing.members)) {
        is_duplicate = true;
        // Keep the one with the lowest port
        if (GetPortFromAddress(server.address) <
            GetPortFromAddress(existing.address)) {
          existing = std::move(server);
        }
        break;
      }
    }

    if (!is_duplicate) {
      group.push_back(std::move(server));
    }
  }

  std::vector<backend::Quake3ServerResult> cached_results;
  model::SearchResponse model_response;
  model_response.result = model::SearchResult::kOk;

  for (auto& pair : grouped_servers) {
    for (auto& server : pair.second) {
      // Sort players: humans first, then bots. Within each group, sort by name
      // ASC.
      std::sort(server.members.begin(), server.members.end(),
                [](const auto& a, const auto& b) {
                  bool a_is_bot = (a.ping == 0);
                  bool b_is_bot = (b.ping == 0);
                  if (a_is_bot != b_is_bot) {
                    return !a_is_bot;  // Human (not bot) comes first
                  }
                  return a.name < b.name;
                });

      int humans = 0;
      for (const auto& member : server.members) {
        if (member.ping > 0)
          humans++;
      }

      model::GameServerLobbyResult entry;
      entry.result_fields = {
          server.address,
          server.game_type,
          server.hostname,
          server.map,
          std::to_string(server.players) + "/" +
              std::to_string(server.max_players),
          std::to_string(humans) + "/" + std::to_string(server.max_players),
          std::to_string(server.ping)};
      entry.metadata = server.metadata;
      model_response.results.lobbies.push_back(std::move(entry));
      cached_results.push_back(std::move(server));
    }
  }

  last_response_results_ = std::move(cached_results);

  if (players_callback) {
    model::GamePlayersResults players_results;
    for (const auto& server : *last_response_results_) {
      int humans = 0;
      for (const auto& member : server.members) {
        if (member.ping > 0)
          humans++;
      }

      for (const auto& member : server.members) {
        players_results.push_back(model::GamePlayersResult{{
            server.address,  // Lobby Id
            member.name,     // Player Id
            "",              // Icon (Favorite)
            std::to_string(member.score),
            member.name,  // Player Name
            std::to_string(member.ping),
            server.hostname,
            server.game_type,
            server.map,
            std::to_string(server.players) + "/" +
                std::to_string(server.max_players),
            std::to_string(humans) + "/" + std::to_string(server.max_players),
        }});
      }
    }
    std::move(players_callback).Run(std::move(players_results));
  }

  SetStatusText(std::string("Found ") +
                std::to_string(model_response.results.lobbies.size()) +
                std::string(" Quake 3 servers"));
  std::move(on_done_callback).Run(std::move(model_response));
}

}  // namespace engine::game
