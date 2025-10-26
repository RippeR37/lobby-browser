#pragma once

#include <memory>

#include "base/threading/thread.h"

#include "engine/backends/auth_token.h"
#include "models/lobby_connector.h"

namespace engine::backend {

// This class, when instantiated, will attempt to connect and reserve a slot in
// a specified lobby and inform user of the progress.
// Once the instance is destroyed, the user is disconnected and everything is
// cleaned up.
//
// All callbacks will be run on the task runner on which this object was
// created.
class EosLobbyConnector : public model::LobbyConnector {
 public:
  EosLobbyConnector(
      std::string deployment_id,
      AuthToken<std::string> access_token,
      std::string lobby_id,
      base::RepeatingCallback<void(std::string)> status_update_cb,
      base::RepeatingCallback<void(int)> progress_update_cb,
      base::RepeatingCallback<void(LobbyStateUpdate)> state_update_cb,
      base::OnceCallback<void(bool)> on_done_callback);
  ~EosLobbyConnector() override;

 private:
  class ClientImpl;

  base::Thread worker_thread_;
  std::unique_ptr<ClientImpl> client_;
};

}  // namespace engine::backend
