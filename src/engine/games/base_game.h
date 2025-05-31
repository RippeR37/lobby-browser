#pragma once

#include "engine/games/game.h"
#include "engine/presenter.h"

namespace engine::game {

class BaseGame : public Game {
 public:
  using SetStatusTextCallback = base::RepeatingCallback<void(std::string)>;
  using ReportMessageCallback = base::RepeatingCallback<
      void(Presenter::MessageType, std::string, std::string)>;

  BaseGame(SetStatusTextCallback set_status_text,
           ReportMessageCallback report_message);
  ~BaseGame() override;

 protected:
  void SetStatusText(std::string);
  void ReportMessage(Presenter::MessageType type,
                     std::string message,
                     std::string details);

 private:
  SetStatusTextCallback set_status_text_;
  ReportMessageCallback report_message_;
};

}  // namespace engine::game
