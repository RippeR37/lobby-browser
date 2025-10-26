#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace engine::backend::eos {

//
// SearchLobbies
//

enum class CriteriaOperator {
  EQUAL,
  NOT_EQUAL,
  ANY_OF,
};

struct SearchLobbiesCriteria {
  std::string key;
  CriteriaOperator op;
  std::variant<int64_t, std::string, std::vector<std::string>> value;
};

struct SearchLobbiesRequest {
  std::vector<SearchLobbiesCriteria> criteria;
  int64_t min_current_players;
  int64_t max_results;
};

struct SearchLobbiesSessionSettings {
  int64_t max_public_players;
  // ...
};

struct SearchLobbiesSession {
  std::string deployment;
  std::string id;
  std::string bucket;
  int64_t total_players;
  int64_t open_public_players;
  std::vector<std::string> public_players;
  SearchLobbiesSessionSettings settings;
  bool started;
  std::map<std::string, std::string> attributes;
  std::string owner;
  int64_t owner_platform_id;
};

struct SearchLobbiesResponse {
  std::vector<SearchLobbiesSession> sessions;
  int64_t count;

  std::string todo_auth_token;
};

//
// FetchUsersInfo
//

using ProductUserId = std::string;

struct FetchUsersInfoRequest {
  std::vector<ProductUserId> product_user_ids;
};

struct ProductUserAccount {
  std::string account_id;
  std::string display_name;
  std::string identity_provider_id;
  std::string last_login;
};

struct ProductUser {
  std::vector<ProductUserAccount> accounts;
};

struct FetchUsersInfoResponse {
  std::map<ProductUserId, ProductUser> product_users;
};

//
// LobbyConnector
//

struct WsJoinRequest {
  std::string request_id;
  std::string lobby_id;
  bool allow_crossplay;
};

struct WsErrorResponse {
  std::string request_id;
  int status_code;
  std::string error_code;
  std::string error_message;
};

struct WsMemberData {
  std::string id;
  std::string display_name;
};

struct WsLobbyInfoResponse {
  std::string request_id;
  std::string owner_id;
  std::string game_mode;
  std::string map;
  std::vector<WsMemberData> members;
  int open_public_players;
  std::string state;
};

struct WsLobbyDataChangeMessage {
  std::string game_mode;
  std::string map;
  std::string owner_name;
  std::string state;
};

struct WsMemberJoinedMessage {
  std::string player_uid;
};

struct WsMemberLeftMessage {
  std::string player_uid;
  std::string reason;
};

struct WsMemberDataChangeMessage {
  std::string player_uid;
  std::string display_name;
};

}  // namespace engine::backend::eos
