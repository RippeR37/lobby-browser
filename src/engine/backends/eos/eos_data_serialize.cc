#include "engine/backends/eos/eos_data_serialize.h"

namespace engine::backend::eos {

void to_json(nlohmann::json& out, const CriteriaOperator& obj) {
  switch (obj) {
    case CriteriaOperator::EQUAL:
      out = nlohmann::json("EQUAL");
      return;

    case CriteriaOperator::NOT_EQUAL:
      out = nlohmann::json("NOT_EQUAL");
      return;
    case CriteriaOperator::ANY_OF:
      out = nlohmann::json("ANY_OF");
      return;
  }
}

void to_json(nlohmann::json& out, const SearchLobbiesCriteria& obj) {
  out = nlohmann::json{
      {"key", obj.key},
      {"op", obj.op},
  };

  if (const auto* value = std::get_if<int64_t>(&obj.value)) {
    out["value"] = *value;
  } else if (const auto* value = std::get_if<std::string>(&obj.value)) {
    out["value"] = *value;
  } else if (const auto* value =
                 std::get_if<std::vector<std::string>>(&obj.value)) {
    out["value"] = *value;
  } else {
    throw nlohmann::json::type_error::create(
        1, "Unexpected type - expected int, string or array", nullptr);
  }
}

void to_json(nlohmann::json& out, const SearchLobbiesRequest& obj) {
  out = nlohmann::json{
      {"criteria", obj.criteria},
      {"minCurrentPlayers", obj.min_current_players},
      {"maxResults", obj.max_results},
  };
}

void from_json(const nlohmann::json& in, SearchLobbiesSessionSettings& obj) {
  obj.max_public_players = in.value("maxPublicPlayers", -1);
}

void from_json(const nlohmann::json& in, SearchLobbiesSession& obj) {
  obj.deployment = in.value("deployment", "");
  obj.id = in.value("id", "");
  obj.bucket = in.value("bucket", "");
  obj.settings = in.value("settings", SearchLobbiesSessionSettings{});
  obj.total_players = in.value("totalPlayers", -1);
  obj.open_public_players = in.value("openPublicPlayers", -1);
  obj.public_players = in.value("publicPlayers", std::vector<std::string>{});
  obj.started = in.value("started", false);
  obj.attributes = in.value("attributes", std::map<std::string, std::string>{});
  obj.owner = in.value("owner", "");
  obj.owner_platform_id = in.value("ownerPlatformId", -1);
}

void from_json(const nlohmann::json& in, SearchLobbiesResponse& obj) {
  obj.sessions = in.value("sessions", std::vector<SearchLobbiesSession>{});
  obj.count = in.value("count", -1);
}

void to_json(nlohmann::json& out, const FetchUsersInfoRequest& obj) {
  out = nlohmann::json{
      {"productUserIds", obj.product_user_ids},
  };
}

void from_json(const nlohmann::json& in, ProductUserAccount& obj) {
  obj.account_id = in.value("accountId", "");
  obj.display_name = in.value("displayName", "");
  obj.identity_provider_id = in.value("identityProviderId", "");
  obj.last_login = in.value("lastLogin", "");
}

void from_json(const nlohmann::json& in, ProductUser& obj) {
  obj.accounts = in.value("accounts", std::vector<ProductUserAccount>{});
}

void from_json(const nlohmann::json& in, FetchUsersInfoResponse& obj) {
  obj.product_users =
      in.value("productUsers", std::map<ProductUserId, ProductUser>{});
}

void to_json(nlohmann::json& out, const WsJoinRequest& obj) {
  out = {
      {"name", "join"},
      {"requestId", obj.request_id},
      {"payload",
       {
           {"lobbyId", obj.lobby_id},
           {"allowCrossplay", obj.allow_crossplay},
       }},
  };
}

void from_json(const nlohmann::json& in, WsErrorResponse& obj) {
  obj.request_id = in.value("requestId", "");

  if (in.contains("payload") && in["payload"].is_object()) {
    const auto& payload = in["payload"];
    obj.status_code = payload.value("statusCode", -1);
    obj.error_code = payload.value("errorCode", "");
    obj.error_message = payload.value("errorMessage", "");
  }
}

void from_json(const nlohmann::json& in, WsLobbyInfoResponse& obj) {
  obj.request_id = in.value("requestId", "");
}

}  // namespace engine::backend::eos
