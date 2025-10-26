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
  if (in.contains("owner") && in["owner"].is_string()) {
    obj.owner = in.value("owner", "");
  }
  if (in.contains("ownerPlatformId") && in["ownerPlatformId"].is_string()) {
    obj.owner_platform_id = in.value("ownerPlatformId", -1);
  }
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

  if (!in.contains("payload") || !in["payload"].is_object()) {
    return;
  }

  const auto& payload = in["payload"];

  if (payload.contains("memberData") && payload["memberData"].is_object()) {
    const auto& outer_member_data = payload["memberData"];

    if (outer_member_data.contains("memberData") &&
        outer_member_data["memberData"].is_object()) {
      const nlohmann::json& inner_member_data = outer_member_data["memberData"];

      // iterate over key-values here
      for (const auto& [puid, member_data] : inner_member_data.items()) {
        bool added = false;

        if (member_data.contains("data") && member_data["data"].is_object()) {
          const auto& data = member_data["data"];
          if (data.contains("DISPLAYNAME_s") &&
              data["DISPLAYNAME_s"].is_string()) {
            obj.members.emplace_back(
                WsMemberData{puid, data["DISPLAYNAME_s"].get<std::string>()});
            added = true;
          }
        }
        if (!added) {
          obj.members.emplace_back(WsMemberData{puid, "<NoPlayerName>"});
        }
      }
    }
  }

  if (payload.contains("publicData") && payload["publicData"].is_object()) {
    const auto& public_data = payload["publicData"];

    if (public_data.contains("owner") && public_data["owner"].is_string()) {
      obj.owner_id = public_data.value("owner", "");
    }
    if (public_data.contains("openPublicPlayers") &&
        public_data["openPublicPlayers"].is_number_integer()) {
      obj.open_public_players = public_data.value("openPublicPlayers", -1);
    }

    if (public_data.contains("attributes") &&
        public_data["attributes"].is_object()) {
      const auto& attributes = public_data["attributes"];

      if (attributes.contains("GAMELABEL_s") &&
          attributes["GAMELABEL_s"].is_string()) {
        obj.game_mode = attributes["GAMELABEL_s"];
      }
      if (attributes.contains("MAPLABEL_s") &&
          attributes["MAPLABEL_s"].is_string()) {
        obj.map = attributes["MAPLABEL_s"];
      }
      if (attributes.contains("STATE_s") && attributes["STATE_s"].is_string()) {
        obj.state = attributes["STATE_s"];
      }
    }
  }
}

void from_json(const nlohmann::json& in, WsLobbyDataChangeMessage& obj) {
  if (!in.contains("payload") || !in["payload"].is_object()) {
    return;
  }
  const auto& payload = in["payload"];

  if (payload.contains("attributes") && payload["attributes"].is_object()) {
    const auto& attributes = payload["attributes"];

    if (attributes.contains("OWNERNAME_s") &&
        attributes["OWNERNAME_s"].is_string()) {
      obj.owner_name = attributes["OWNERNAME_s"];
    }
    if (attributes.contains("GAMELABEL_s") &&
        attributes["GAMELABEL_s"].is_string()) {
      obj.game_mode = attributes["GAMELABEL_s"];
    }
    if (attributes.contains("MAPLABEL_s") &&
        attributes["MAPLABEL_s"].is_string()) {
      obj.map = attributes["MAPLABEL_s"];
    }
    if (attributes.contains("STATE_s") && attributes["STATE_s"].is_string()) {
      obj.state = attributes["STATE_s"];
    }
  }
}

void from_json(const nlohmann::json& in, WsMemberJoinedMessage& obj) {
  if (in.contains("payload") && in["payload"].is_object()) {
    const auto& payload = in["payload"];
    obj.player_uid = payload.value("puid", "");
  }
}

void from_json(const nlohmann::json& in, WsMemberLeftMessage& obj) {
  if (in.contains("payload") && in["payload"].is_object()) {
    const auto& payload = in["payload"];
    obj.player_uid = payload.value("puid", "");
    obj.reason = payload.value("reason", "");
  }
}

void from_json(const nlohmann::json& in, WsMemberDataChangeMessage& obj) {
  if (in.contains("payload") && in["payload"].is_object()) {
    const auto& payload = in["payload"];
    obj.player_uid = payload.value("puid", "");

    if (payload.contains("memberData") && payload["memberData"].is_object()) {
      const auto& member_data = payload["memberData"];

      if (member_data.contains("data") && member_data["data"].is_object()) {
        const auto& data = member_data["data"];

        if (data.contains("DISPLAYNAME_s") &&
            data["DISPLAYNAME_s"].is_string()) {
          obj.display_name = data.value("DISPLAYNAME_s", "");
        }
      }
    }
  }
}

}  // namespace engine::backend::eos
