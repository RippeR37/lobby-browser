#pragma once

#include "engine/backends/nakama/nakama_data.h"

#include "nlohmann/json.hpp"

namespace engine::backend::nakama {

void to_json(nlohmann::json& out, const AuthRequest& obj);
void to_json(nlohmann::json& out, const MatchQuery& obj);
void to_json(nlohmann::json& out, const LobbySearchRequest& obj);

void from_json(const nlohmann::json& in, AuthResponse& obj);
void from_json(const nlohmann::json& in, MatchData& obj);
void from_json(const nlohmann::json& in, ListServerResult& obj);
void from_json(const nlohmann::json& in, LobbySearchResponse& obj);

}  // namespace engine::backend::nakama
