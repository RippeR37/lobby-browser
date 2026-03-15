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

  // Game
  bool IsPlayerInFavorites(std::string player_id) const override;
  void AddPlayerToFavorites(std::string player_id) override;
  void RemovePlayerFromFavorites(std::string player_id) override;

  model::GameConfigDescriptor GetConfigDescriptor() const override;
  void UpdateConfigOption(std::string key, std::string value) override;
  void AddConfigListItem(std::string key,
                         std::vector<std::string> fields) override;
  void RemoveConfigListItem(std::string key, std::string item_id) override;
  bool CommitConfig(model::GameConfigDescriptor descriptor) override;

  void LaunchGame(std::string server_address) override;

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
