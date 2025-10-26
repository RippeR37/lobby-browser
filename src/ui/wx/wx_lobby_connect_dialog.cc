#include "ui/wx/wx_lobby_connect_dialog.h"

#include "wx/statline.h"

namespace ui::wx {

WxLobbyConnectDialog::WxLobbyConnectDialog(
    wxWindow* parent,
    std::string lobby_id,
    std::string owner,
    model::LobbyConnectorCreateCallback create_lobby_connector)
    : wxDialog(parent,
               wxID_ANY,
               "Connecting to lobby (" + owner + ")",
               wxDefaultPosition,
               wxSize(500, 300)),
      weak_factory_(this) {
  weak_this_ = weak_factory_.GetWeakPtr();

  wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

  // Grid sizer for two-column status info
  auto* gridSizer = new wxGridSizer(1, 2, 5, 10);

  gridSizer->Add(new wxStaticText(this, wxID_ANY, "Status:"), 0,
                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
  status_text_ = new wxStaticText(this, wxID_ANY, "Preparing...");
  gridSizer->Add(status_text_, 0, wxEXPAND);

  gauge_ = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize,
                       wxGA_HORIZONTAL);
  button_ = new wxButton(this, wxID_ANY, "Cancel");

  mainSizer->Add(gridSizer, 0, wxALL | wxEXPAND, 10);
  mainSizer->Add(gauge_, 0, wxALL | wxEXPAND, 10);
  mainSizer->AddStretchSpacer(1);
  mainSizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
  mainSizer->Add(button_, 0, wxALL | wxALIGN_CENTER, 10);

  SetSizerAndFit(mainSizer);
  Center();

  button_->Bind(wxEVT_BUTTON, &WxLobbyConnectDialog::OnActionButtonClick, this);

  lobby_connector_ = create_lobby_connector.Run(
      lobby_id,
      base::BindRepeating(&WxLobbyConnectDialog::UpdateText, weak_this_),
      base::BindRepeating(&WxLobbyConnectDialog::UpdateGauge, weak_this_),
      base::BindOnce(&WxLobbyConnectDialog::OnDone, weak_this_));
}

WxLobbyConnectDialog::~WxLobbyConnectDialog() = default;

void WxLobbyConnectDialog::UpdateText(std::string new_text) {
  status_text_->SetLabel(new_text);
}

void WxLobbyConnectDialog::UpdateGauge(int new_value) {
  gauge_->SetValue(new_value);
}

void WxLobbyConnectDialog::OnDone(bool success) {
  DCHECK(!result_);
  result_ = success;

  if (HasSucceeded()) {
    UpdateText("Connected");
    UpdateGauge(100);
    SetTitle("Connected to lobby");
    button_->SetLabel("Disconnect");
  } else {
    UpdateText("Failed");
    UpdateGauge(0);
    SetTitle("Failed to connect to lobby");
    button_->SetLabel("Close");
    lobby_connector_.reset();
  }

  wxBell();
}

void WxLobbyConnectDialog::OnActionButtonClick(wxCommandEvent&) {
  lobby_connector_.reset();
  UpdateText("Cancelled");
  UpdateGauge(0);
  SetTitle("Connection cancelled");
  EndModal(wxID_CANCEL);
}

bool WxLobbyConnectDialog::HasCompleted() const {
  return result_.has_value();
}

bool WxLobbyConnectDialog::HasSucceeded() const {
  return HasCompleted() && *result_;
}

bool WxLobbyConnectDialog::HasFailed() const {
  return HasCompleted() && !(*result_);
}

}  // namespace ui::wx
