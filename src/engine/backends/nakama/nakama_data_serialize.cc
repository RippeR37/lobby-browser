#include "engine/backends/nakama/nakama_data_serialize.h"

namespace engine::backend::nakama {

void to_json(nlohmann::json& out, const AuthRequest& obj) {
  out = {
      {"token", obj.token},
  };
}

void to_json(nlohmann::json& out, const MatchQuery& obj) {
  out = {
      {"lobbyType", obj.lobby_type},
      {"modLobby", obj.mod_lobby},
      {"maxPlayerNum", obj.max_player_num},
      {"currentPlayerNum", obj.current_player_num},
      {"mapTag", obj.map_tag},
      {"modeTag", obj.mode_tag},
      {"loadoutTag", obj.loadout_tag},
      {"region", obj.region},
      {"ownerName", obj.owner_name},
      {"password", obj.password},
      {"gameType", obj.game_type},
      {"serverMessage", obj.server_message},
      {"dS_Address", obj.ds_address},
      {"modType", obj.mod_type},
  };
}

void to_json(nlohmann::json& out, const LobbySearchRequest& obj) {
  out = {
      {"matchQuery", obj.match_query},
  };
}

void from_json(const nlohmann::json& in, AuthResponse& obj) {
  obj.token = in.value("token", "");
  obj.refresh_token = in.value("refresh_token", "");
}

void from_json(const nlohmann::json& in, MatchData& obj) {
  obj.lobby_type = in.value("LobbyType", -1);
  obj.mod_lobby = in.value("ModLobby", -1);
  obj.max_player_num = in.value("MaxPlayerNum", -1);
  obj.current_player_num = in.value("CurrentPlayerNum", -1);
  obj.map_tag = in.value("MapTag", "");
  obj.mode_tag = in.value("ModeTag", "");
  obj.loadout_tag = in.value("LoadoutTag", "");
  obj.region = in.value("Region", "");
  obj.owner_name = in.value("OwnerName", "");
  obj.password_protected = in.value("Password", 0) != 0;
  obj.game_type = in.value("GameType", "");
  obj.server_message = in.value("ServerMessage", "");
  obj.ds_address = in.value("DS_Address", "");
  obj.mod_type = in.value("ModType", -1);
  obj.oculus_lobby_session_id = in.value("OculusLobbySessionId", "");
}

void from_json(const nlohmann::json& in, ListServerResult& obj) {
  obj.lobby_id = in.value("Lobby_id", "");
  obj.match_data = in.value("MatchData", MatchData{});
}

void from_json(const nlohmann::json& in, LobbySearchResponse& obj) {
  obj.num_results = in.value("NumResults", -1);
  obj.listserver_results =
      in.value("ListServer_Results", std::vector<ListServerResult>{});
}

}  // namespace engine::backend::nakama
