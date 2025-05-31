#pragma once

#include <map>
#include <string>
#include <vector>

namespace engine::backend::nakama {

//
// Auth
//

struct AuthRequest {
  std::string token;
};

struct AuthResponse {
  std::string token;
  std::string refresh_token;
};

//
// Lobby search
//

struct MatchQuery {
  int64_t lobby_type;
  bool mod_lobby;
  int64_t max_player_num;
  int64_t current_player_num;
  std::string map_tag;
  std::string mode_tag;
  std::string loadout_tag;
  std::string region;
  std::string owner_name;
  int64_t password;
  std::string game_type;
  std::string server_message;
  std::string ds_address;
  int64_t mod_type;
};

struct LobbySearchRequest {
  MatchQuery match_query;
};

struct MatchData {
  int64_t lobby_type;
  bool mod_lobby;
  int64_t max_player_num;
  int64_t current_player_num;
  std::string map_tag;
  std::string mode_tag;
  std::string loadout_tag;
  std::string region;
  std::string owner_name;
  bool password_protected;
  std::string game_type;
  std::string server_message;
  std::string ds_address;
  int64_t mod_type;
  std::string oculus_lobby_session_id;
};

struct ListServerResult {
  std::string lobby_id;
  MatchData match_data;
};

struct LobbySearchResponse {
  int64_t num_results;
  std::vector<ListServerResult> listserver_results;
};

}  // namespace engine::backend::nakama
