#pragma once

#include "engine/backends/eos/eos_data.h"

#include "nlohmann/json.hpp"

namespace engine::backend::eos {

void to_json(nlohmann::json& out, const CriteriaOperator& obj);
void to_json(nlohmann::json& out, const SearchLobbiesCriteria& obj);
void to_json(nlohmann::json& out, const SearchLobbiesRequest& obj);

void from_json(const nlohmann::json& in, SearchLobbiesSessionSettings& obj);
void from_json(const nlohmann::json& in, SearchLobbiesSession& obj);
void from_json(const nlohmann::json& in, SearchLobbiesResponse& obj);

void to_json(nlohmann::json& out, const FetchUsersInfoRequest& obj);

void from_json(const nlohmann::json& in, ProductUserAccount& obj);
void from_json(const nlohmann::json& in, ProductUser& obj);
void from_json(const nlohmann::json& in, FetchUsersInfoResponse& obj);

void to_json(nlohmann::json& out, const SearchUsersRequest& obj);

void from_json(const nlohmann::json& in, SearchUsersPlayer& obj);
void from_json(const nlohmann::json& in, SearchUsersResponse& obj);

}  // namespace engine::backend::eos
