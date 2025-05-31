#pragma once

#include <string>

#include "base/callback.h"

#include "steam_api.h"

namespace engine::backend::steam {

class SteamManager {
 public:
  struct Result {
    std::string ticket;
    std::string error_title;
    std::string error_description;
  };

  SteamManager(bool initialize);
  ~SteamManager();

  bool IsInitialized() const;
  void InitializeIfNeeded();
  void ShutdownIfNeeded();

  bool IsLoggedIn() const;
  uint64 GetUserId() const;
  std::string GetUserName() const;

  bool VerifySteamId() const;

  std::string GetAuthSessionTicket() const;

  // If fails or is canceled, will run an empty string
  void AsyncRequestEncryptedAppTicket(
      base::OnceCallback<void(Result)> on_done_callback);
  void CancelPendingAppTicketRequestIfNeeded();

 private:
  void OnEncryptedAppTicketResponse(
      EncryptedAppTicketResponse_t* ticket_response,
      bool io_failure);

  bool initialized_;
  CCallResult<SteamManager, EncryptedAppTicketResponse_t>
      encrypted_app_ticket_response_call_result_;
  base::OnceCallback<void(Result)> pending_encrypted_app_ticket_response_cb_;
};

}  // namespace engine::backend::steam
