#pragma once

#include <memory>
#include <string>

#include "base/callback.h"

namespace model {

class LobbyConnector {
 public:
  virtual ~LobbyConnector() = default;
};

using LobbyConnectorCreateCallback =
    base::RepeatingCallback<std::unique_ptr<LobbyConnector>(
        std::string,
        base::RepeatingCallback<void(std::string)>,
        base::RepeatingCallback<void(int)>,
        base::OnceCallback<void(bool)>)>;

}  // namespace model
