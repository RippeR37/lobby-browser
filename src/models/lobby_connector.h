#pragma once

#include <memory>
#include <string>

#include "base/callback.h"

namespace model {

class LobbyConnector {
 public:
  struct LobbyStateUpdate {
    std::optional<std::string> owner;
    std::optional<std::string> game_mode;
    std::optional<std::string> map;
    std::optional<std::string> state;
    std::optional<std::string> players;
  };

  virtual ~LobbyConnector() = default;
};

using LobbyConnectorCreateCallback =
    base::RepeatingCallback<std::unique_ptr<LobbyConnector>(
        std::string,
        base::RepeatingCallback<void(std::string)>,
        base::RepeatingCallback<void(int)>,
        base::RepeatingCallback<void(LobbyConnector::LobbyStateUpdate)>,
        base::OnceCallback<void(bool)>)>;

}  // namespace model
