#include "engine/games/base_game.h"

namespace engine::game {

BaseGame::BaseGame(SetStatusTextCallback set_status_text,
                   ReportMessageCallback report_message)
    : set_status_text_(std::move(set_status_text)),
      report_message_(std::move(report_message)) {}

BaseGame::~BaseGame() = default;

bool BaseGame::IsPlayerInFavorites(std::string player_id) const {
  (void)player_id;
  return false;
}

void BaseGame::AddPlayerToFavorites(std::string player_id) {
  (void)player_id;
  return;
}

void BaseGame::RemovePlayerFromFavorites(std::string player_id) {
  (void)player_id;
}

model::GameConfigDescriptor BaseGame::GetConfigDescriptor() const {
  return {};
}

void BaseGame::UpdateConfigOption(std::string key, std::string value) {
  (void)key;
  (void)value;
}

void BaseGame::AddConfigListItem(std::string key,
                                 std::vector<std::string> fields) {
  (void)key;
  (void)fields;
}

void BaseGame::RemoveConfigListItem(std::string key, std::string item_id) {
  (void)key;
  (void)item_id;
}

bool BaseGame::CommitConfig(model::GameConfigDescriptor descriptor) {
  (void)descriptor;
  return false;
}

void BaseGame::LaunchGame(std::string server_address) {
  (void)server_address;
}

void BaseGame::SetStatusText(std::string status_text) {
  set_status_text_.Run(std::move(status_text));
}

void BaseGame::ReportMessage(Presenter::MessageType type,
                             std::string message,
                             std::string details) {
  report_message_.Run(type, std::move(message), std::move(details));
}

}  // namespace engine::game
