#pragma once

#include <string>
#include <vector>

#include "models/config.h"
#include "models/game.h"

namespace engine {

class Presenter {
 public:
  enum class MessageType {
    kInfo,
    kWarning,
    kError,
    kFatalError,
  };

  virtual ~Presenter() = default;

  virtual std::string GetPresenterName() const = 0;

  virtual void Initialize(model::AppConfig app_config,
                          model::UiConfig ui_config,
                          std::vector<model::Game> game_models) = 0;

  virtual void SetStatusText(std::string status_text) = 0;

  virtual void ReportMessage(MessageType type,
                             std::string message,
                             std::string details) = 0;
};

}  // namespace engine
