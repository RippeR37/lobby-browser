#pragma once

#include "base/memory/weak_ptr.h"

#include "engine/backends/eos/eos_lobby_backend.h"
#include "engine/backends/pavlov/pavlov_data.h"

namespace engine::backend {

class PavlovLobbyBackend : public EosLobbyBackend {
 public:
  PavlovLobbyBackend(std::string steam_auth_backend_command);
  ~PavlovLobbyBackend() override;

  void SearchUsers(pavlov::SearchUsersRequest request,
                   base::OnceCallback<void(Result, pavlov::SearchUsersResponse)>
                       on_done_callback);

 protected:
  void DoSearchUsers(
      std::string json_request,
      base::OnceCallback<void(Result, pavlov::SearchUsersResponse)>
          on_done_callback,
      std::optional<Result> error_result);
  void OnSearchUsersResponse(
      base::OnceCallback<void(Result, pavlov::SearchUsersResponse)>
          on_done_callback,
      base::net::ResourceResponse response);

  base::WeakPtr<PavlovLobbyBackend> pavlov_weak_this_;
  base::WeakPtrFactory<PavlovLobbyBackend> pavlov_weak_factory_;
};

}  // namespace engine::backend
