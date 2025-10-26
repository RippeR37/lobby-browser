#pragma once

#include <optional>

#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
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

  void UpdateText(std::string new_text);
  void UpdateGauge(int new_value);
  void OnDone(bool success);
  void OnActionButtonClick(wxCommandEvent&);

  bool HasCompleted() const;
  bool HasSucceeded() const;
  bool HasFailed() const;

  void AutoCloseOnDoneTimeout();

  bool as_modal_;
  wxGauge* gauge_;
  wxStaticText* status_text_;
  wxButton* button_;
  std::optional<bool> result_;

  std::unique_ptr<model::LobbyConnector> lobby_connector_;

  base::WeakPtr<WxLobbyConnectDialog> weak_this_;
  base::WeakPtrFactory<WxLobbyConnectDialog> weak_factory_;
};

}  // namespace ui::wx
