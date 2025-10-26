#pragma once

#include <optional>
#include <unordered_map>

#include "engine/backends/eos/eos_data.h"
#include "engine/games/pavlov/pavlov_data.h"

namespace engine::game::pavlov {

class PavlovPlayersDataStore {
 public:
  std::optional<PavlovPlayerData> GetCachedDataFor(
      const std::string& user_id) const;

  // Returns list of user IDs that we should fetch details for
  std::vector<std::string> OnNewUserIds(
      const std::vector<std::string>& new_user_ids);

  // Returns list of user IDs that data became known after this call
  void ProcessNewUserData(backend::eos::FetchUsersInfoResponse response);

 private:
  bool HasCachedDataFor(const std::string& user_id) const;

  std::unordered_map<std::string, PavlovPlayerData> cached_data_;
};

}  // namespace engine::game::pavlov
