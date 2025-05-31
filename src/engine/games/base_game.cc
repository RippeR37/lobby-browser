#include "engine/games/base_game.h"

namespace engine::game {

BaseGame::BaseGame(SetStatusTextCallback set_status_text,
                   ReportMessageCallback report_message)
    : set_status_text_(std::move(set_status_text)),
      report_message_(std::move(report_message)) {}

BaseGame::~BaseGame() = default;

void BaseGame::SetStatusText(std::string status_text) {
  set_status_text_.Run(std::move(status_text));
}

void BaseGame::ReportMessage(Presenter::MessageType type,
                             std::string message,
                             std::string details) {
  report_message_.Run(type, std::move(message), std::move(details));
}

}  // namespace engine::game
