#pragma once

#include <optional>

#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "wx/wx.h"

#include "models/lobby_connector.h"
#include "models/search.h"

namespace ui::wx {

class WxLobbyConnectDialog : public wxDialog {
 public:
  WxLobbyConnectDialog(
      wxWindow* parent,
      std::string lobby_id,
      std::string game_mode,
      std::string owner,
      model::LobbyConnectorCreateCallback create_lobby_connector,
      bool as_modal);
  ~WxLobbyConnectDialog() override;

 private:
  void PositionRandomlyRelativeToParent();
  void PeriodicUpdateState();

  void UpdateText(std::string new_text);
  void UpdateGauge(int new_value);
  void UpdateState(model::LobbyConnector::LobbyStateUpdate updated_state);
  void OnDone(bool success);
  void OnActionButtonClick(wxCommandEvent&);
  void OnAutoDisconnectChanged(wxCommandEvent&);

  bool HasCompleted() const;
  bool HasSucceeded() const;
  bool HasFailed() const;

  void AutoCloseOnDoneTimeout();

  bool as_modal_;
  wxStaticText* owner_text_;
  wxStaticText* game_mode_text_;
  wxStaticText* map_text_;
  wxStaticText* players_text_;
  wxStaticText* lobby_state_text_;
  wxGauge* gauge_;
  wxStaticText* status_text_;
  wxCheckBox* auto_disconnect_checkbox_;
  wxStaticText* auto_disconnect_time_left_text_;
  wxButton* button_;
  std::optional<bool> result_;
  std::optional<base::Time> auto_disconnect_time_;

  std::unique_ptr<model::LobbyConnector> lobby_connector_;

  base::WeakPtr<WxLobbyConnectDialog> weak_this_;
  base::WeakPtrFactory<WxLobbyConnectDialog> weak_factory_;
};

}  // namespace ui::wx
