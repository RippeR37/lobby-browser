#include "engine/games/pavlov/pavlov_players_data_store.h"

#include "base/logging.h"

#include "nlohmann/json.hpp"

namespace engine::game::pavlov {

namespace {
std::optional<PavlovUserPlatform> StringToUserPlatform(const std::string& str) {
  if (str == "steam") {
    return PavlovUserPlatform::kSteam;
  }
  if (str == "psn") {
    return PavlovUserPlatform::kPlayStation;
  }
  return {};
}
}  // namespace

std::optional<PavlovPlayerData> PavlovPlayersDataStore::GetCachedDataFor(
    const std::string& user_id) const {
  if (HasCachedDataFor(user_id)) {
    return cached_data_.find(user_id)->second;
  }
  return std::nullopt;
}

std::vector<std::string> PavlovPlayersDataStore::OnNewUserIds(
    const std::vector<std::string>& new_user_ids) {
  std::vector<std::string> user_ids_to_fetch_details;

  for (const auto& user_id : new_user_ids) {
    if (HasCachedDataFor(user_id)) {
      continue;
    }
    user_ids_to_fetch_details.push_back(user_id);
  }

  return user_ids_to_fetch_details;
}

// Returns list of user IDs that data became known after this call
void PavlovPlayersDataStore::ProcessNewUserData(
    backend::eos::FetchUsersInfoResponse response) {
  for (const auto& user_info : response.product_users) {
    if (user_info.second.accounts.empty()) {
      continue;
    }

    cached_data_[user_info.first] = PavlovPlayerData{
        user_info.second.accounts.front().account_id,
        user_info.second.accounts.front().display_name,
        StringToUserPlatform(
            user_info.second.accounts.front().identity_provider_id),
        user_info.second.accounts.front().account_id,
        user_info.second.accounts.front().last_login,
    };
  }
}

bool PavlovPlayersDataStore::HasCachedDataFor(
    const std::string& user_id) const {
  return cached_data_.count(user_id) > 0;
}

}  // namespace engine::game::pavlov
