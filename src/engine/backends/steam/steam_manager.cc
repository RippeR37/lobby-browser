#include "engine/backends/steam/steam_manager.h"

#include <array>
#include <iomanip>
#include <set>

#include "base/logging.h"

#include "steam_api.h"

namespace engine::backend::steam {

namespace {
std::string ArrayToHexString(const uint8_t* data, size_t length) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0');

  for (size_t i = 0; i < length; ++i) {
    oss << std::setw(2) << static_cast<int>(data[i]);
  }

  return oss.str();
}
}  // namespace

SteamManager::SteamManager(bool initialize) : initialized_(false) {
  if (initialize) {
    InitializeIfNeeded();
  }
}

SteamManager::~SteamManager() {
  CancelPendingAppTicketRequestIfNeeded();
  ShutdownIfNeeded();
}

bool SteamManager::IsInitialized() const {
  return initialized_;
}

void SteamManager::InitializeIfNeeded() {
  if (IsInitialized()) {
    return;
  }

  if (!SteamAPI_Init()) {
    LOG(ERROR) << "Failed to initialize Steam API";
    return;
  }

  LOG(INFO) << "Steam API initialized";
  initialized_ = true;
}

void SteamManager::ShutdownIfNeeded() {
  if (IsInitialized()) {
    SteamAPI_Shutdown();
    LOG(INFO) << "Steam API deinitialized";
    initialized_ = false;
  }
}

bool SteamManager::IsLoggedIn() const {
  return IsInitialized() && SteamUser() && SteamUser()->BLoggedOn();
}

uint64 SteamManager::GetUserId() const {
  if (!IsLoggedIn()) {
    return 0;
  }

  return SteamUser()->GetSteamID().ConvertToUint64();
}

std::string SteamManager::GetUserName() const {
  if (!IsLoggedIn()) {
    return "";
  }

  return SteamFriends()->GetPersonaName();
}

bool SteamManager::VerifySteamId() const {
  return GetUserId() > 0;
}

std::string SteamManager::GetAuthSessionTicket() const {
  std::array<uint8, 2 * 4096 + 1> ticket_buffer{};
  uint32 ticket_lenght = 0;
  auto identity = SteamNetworkingIdentity{};
  identity.SetSteamID(SteamUser()->GetSteamID());
  SteamUser()->GetAuthSessionTicket(ticket_buffer.data(), ticket_buffer.size(),
                                    &ticket_lenght, &identity);
  return ArrayToHexString(ticket_buffer.data(), ticket_lenght);
}

void SteamManager::AsyncRequestEncryptedAppTicket(
    base::OnceCallback<void(Result)> on_done_callback) {
  if (!IsLoggedIn()) {
    LOG(ERROR) << "User is not logged in, can't request app ticket!";
    std::move(on_done_callback)
        .Run(Result{"", "Steam authentication failed",
                    "Failed to authenticate with Steam - user not logged in"});
    return;
  }

  if (!VerifySteamId()) {
    std::move(on_done_callback)
        .Run(Result{
            "", "Steam authentication failed",
            "Failed to authenticate with Steam - user is not logged in"});
  }

  CancelPendingAppTicketRequestIfNeeded();

  pending_encrypted_app_ticket_response_cb_ = std::move(on_done_callback);

  SteamAPICall_t api_call = SteamUser()->RequestEncryptedAppTicket(nullptr, 0);
  encrypted_app_ticket_response_call_result_.Set(
      api_call, this, &SteamManager::OnEncryptedAppTicketResponse);
}

void SteamManager::CancelPendingAppTicketRequestIfNeeded() {
  if (encrypted_app_ticket_response_call_result_.IsActive()) {
    encrypted_app_ticket_response_call_result_.Cancel();
  }

  if (pending_encrypted_app_ticket_response_cb_) {
    std::move(pending_encrypted_app_ticket_response_cb_)
        .Run(Result{"", "Steam authentication cancelled",
                    "Steam authentication was cancelled"});
  }
}

void SteamManager::OnEncryptedAppTicketResponse(
    EncryptedAppTicketResponse_t* ticket_response,
    bool io_failure) {
  if (!ticket_response) {
    LOG(ERROR) << "Failed to get valid ticket response, IO failure: "
               << io_failure;
    if (pending_encrypted_app_ticket_response_cb_) {
      std::move(pending_encrypted_app_ticket_response_cb_)
          .Run(Result{"", "Steam authentication failed",
                      "Received invalid response data from Steam API"});
    }
    return;
  }

  if (io_failure) {
    LOG(ERROR) << "There has been an IO Failure when requesting the "
                  "Encrypted App Ticket.\n";
    if (pending_encrypted_app_ticket_response_cb_) {
      std::move(pending_encrypted_app_ticket_response_cb_)
          .Run(Result{"", "Steam authentication failed", "IO Failure"});
    }
    return;
  }

  if (ticket_response->m_eResult != k_EResultOK) {
    LOG(ERROR) << "GetTicketForWebApiResponse_t callback failed. Error: "
               << static_cast<int>(ticket_response->m_eResult);
    if (pending_encrypted_app_ticket_response_cb_) {
      std::move(pending_encrypted_app_ticket_response_cb_)
          .Run(Result{"", "Steam authentication failure",
                      "Received error response from Steam API: " +
                          std::to_string(ticket_response->m_eResult)});
    }
    return;
  }

  if (!pending_encrypted_app_ticket_response_cb_) {
    LOG(ERROR) << "Received app ticket but no pending response! Ignoring...";
    return;
  }

  std::array<uint8, 2 * 4096 + 1> ticket_buffer{};
  uint32 ticket_lenght = 0;
  if (!SteamUser()->GetEncryptedAppTicket(
          ticket_buffer.data(), ticket_buffer.size(), &ticket_lenght)) {
    LOG(ERROR) << "Failed to retrieve Auth Ticket\n";

    std::move(pending_encrypted_app_ticket_response_cb_)
        .Run(Result{"", "Steam authentication failure",
                    "Failed to receive authentication data from Steam API"});
    return;
  }

  auto result = ArrayToHexString(ticket_buffer.data(), ticket_lenght);
  std::move(pending_encrypted_app_ticket_response_cb_)
      .Run(Result{result, "", ""});
}

}  // namespace engine::backend::steam
