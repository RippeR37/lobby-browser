#include "ui/wx/wx_lobby_connect_dialog.h"

#include <random>

#include "base/threading/sequenced_task_runner_handle.h"
#include "wx/statline.h"

namespace ui::wx {

namespace {
const auto kModalStyle = wxDEFAULT_DIALOG_STYLE;
const auto kModelessStyle = wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP;

const auto kPeriodicUpdateInterval = base::Milliseconds(250);
const auto kConnectedTimeout = base::Minutes(5);

std::string TimeDeltaToHumanReadableString(base::TimeDelta delta) {
  // We know it's at most minutes, so we can simplify this for now

  const auto minutes = delta.InMinutes();
  const auto seconds = delta.InSeconds() - (minutes * 60);
  const auto* seconds_leading_zero = seconds < 10 ? "0" : "";
  return std::to_string(minutes) + ":" + seconds_leading_zero +
         std::to_string(seconds);
}
}  // namespace

WxLobbyConnectDialog::WxLobbyConnectDialog(
    wxWindow* parent,
    std::string lobby_id,
    std::string game_mode,
    std::string owner,
    model::LobbyConnectorCreateCallback create_lobby_connector,
    bool as_modal)
    : wxDialog(parent,
               wxID_ANY,
               "Connecting to lobby",
               wxDefaultPosition,
               wxSize(500, 300),
               as_modal ? kModalStyle : kModelessStyle),
      as_modal_(as_modal),
      weak_factory_(this) {
  weak_this_ = weak_factory_.GetWeakPtr();

  wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

  // Grid sizer for two-column status info
  auto* gridSizer = new wxGridSizer(3, 2, 5, 10);

  gridSizer->Add(new wxStaticText(this, wxID_ANY, "Owner:"), 0,
                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
  owner_text_ = new wxStaticText(this, wxID_ANY, owner);
  gridSizer->Add(owner_text_, 0, wxALIGN_LEFT);

  gridSizer->Add(new wxStaticText(this, wxID_ANY, "Game mode:"), 0,
                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
  game_mode_text_ = new wxStaticText(this, wxID_ANY, game_mode);
  gridSizer->Add(game_mode_text_, 0, wxALIGN_LEFT);

  gridSizer->Add(new wxStaticText(this, wxID_ANY, "Map:"), 0,
                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
  map_text_ = new wxStaticText(this, wxID_ANY, "");
  gridSizer->Add(map_text_, 0, wxALIGN_LEFT);

  gridSizer->Add(new wxStaticText(this, wxID_ANY, "Players:"), 0,
                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
  players_text_ = new wxStaticText(this, wxID_ANY, "?/?");
  gridSizer->Add(players_text_, 0, wxALIGN_LEFT);

  gridSizer->Add(new wxStaticText(this, wxID_ANY, "Lobby state:"), 0,
                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
  lobby_state_text_ = new wxStaticText(this, wxID_ANY, "?");
  gridSizer->Add(lobby_state_text_, 0, wxALIGN_LEFT);

  // Gap
  gridSizer->Add(new wxStaticText(this, wxID_ANY, ""), 0, wxALIGN_LEFT);
  gridSizer->Add(new wxStaticText(this, wxID_ANY, ""), 0, wxALIGN_LEFT);

  gridSizer->Add(new wxStaticText(this, wxID_ANY, "Status:"), 0,
                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
  status_text_ = new wxStaticText(this, wxID_ANY, "Preparing...");
  gridSizer->Add(status_text_, 0, wxEXPAND);

  // Gap
  gridSizer->Add(new wxStaticText(this, wxID_ANY, ""), 0, wxALIGN_LEFT);
  gridSizer->Add(new wxStaticText(this, wxID_ANY, ""), 0, wxALIGN_LEFT);

  // Auto disconnect fields
  gridSizer->Add(new wxStaticText(this, wxID_ANY, "Auto disconnect:"), 0,
                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
  auto_disconnect_checkbox_ = new wxCheckBox(this, wxID_ANY, "");
  auto_disconnect_checkbox_->SetValue(true);
  gridSizer->Add(auto_disconnect_checkbox_, 0, wxALIGN_LEFT);
  auto_disconnect_checkbox_->Bind(
      wxEVT_CHECKBOX, &WxLobbyConnectDialog::OnAutoDisconnectChanged, this);

  gridSizer->Add(new wxStaticText(this, wxID_ANY, "Disconnect in:"), 0,
                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
  auto_disconnect_time_left_text_ = new wxStaticText(this, wxID_ANY, "-:--");
  gridSizer->Add(auto_disconnect_time_left_text_, 0, wxALIGN_LEFT);

  // Progress bar & action button
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

  if (!as_modal) {
    PositionRandomlyRelativeToParent();
  }

  button_->Bind(wxEVT_BUTTON, &WxLobbyConnectDialog::OnActionButtonClick, this);

  lobby_connector_ = create_lobby_connector.Run(
      lobby_id,
      base::BindRepeating(&WxLobbyConnectDialog::UpdateText, weak_this_),
      base::BindRepeating(&WxLobbyConnectDialog::UpdateGauge, weak_this_),
      base::BindRepeating(&WxLobbyConnectDialog::UpdateState, weak_this_),
      base::BindOnce(&WxLobbyConnectDialog::OnDone, weak_this_));

  PeriodicUpdateState();
}

WxLobbyConnectDialog::~WxLobbyConnectDialog() = default;

void WxLobbyConnectDialog::PositionRandomlyRelativeToParent() {
  auto center_position = GetPosition();

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(-150, 150);

  int offsetX = dist(gen);
  int offsetY = dist(gen);

  // Set new position
  wxPoint dialogPos = center_position + wxPoint(offsetX, offsetY);
  SetPosition(dialogPos);
}

void WxLobbyConnectDialog::PeriodicUpdateState() {
  // First trigger next update
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&WxLobbyConnectDialog::PeriodicUpdateState, weak_this_),
      kPeriodicUpdateInterval);

  if (!HasSucceeded()) {
    return;
  }

  if (!auto_disconnect_checkbox_->IsChecked()) {
    return;
  }
  if (!auto_disconnect_time_) {
    return;
  }

  auto time_left = *auto_disconnect_time_ - base::Time::Now();
  time_left = std::max(time_left, base::TimeDelta{});

  if (time_left.IsZero()) {
    AutoCloseOnDoneTimeout();
    return;
  }

  auto_disconnect_time_left_text_->SetLabelText(
      TimeDeltaToHumanReadableString(time_left));
}

void WxLobbyConnectDialog::UpdateText(std::string new_text) {
  status_text_->SetLabelText(new_text);
}

void WxLobbyConnectDialog::UpdateGauge(int new_value) {
  gauge_->SetValue(new_value);
}

void WxLobbyConnectDialog::UpdateState(
    model::LobbyConnector::LobbyStateUpdate updated_state) {
  if (updated_state.owner) {
    owner_text_->SetLabelText(*updated_state.owner);
  }
  if (updated_state.game_mode) {
    game_mode_text_->SetLabelText(*updated_state.game_mode);
  }
  if (updated_state.map) {
    map_text_->SetLabelText(*updated_state.map);
  }
  if (updated_state.players) {
    players_text_->SetLabelText(*updated_state.players);
  }
  if (updated_state.state) {
    lobby_state_text_->SetLabelText(*updated_state.state);
  }
}

void WxLobbyConnectDialog::OnDone(bool success) {
  DCHECK(!result_);
  result_ = success;

  if (HasSucceeded()) {
    UpdateText("Connected");
    UpdateGauge(100);
    SetTitle("Connected to lobby");
    button_->SetLabel("Disconnect");

    auto_disconnect_time_ = base::Time::Now() + kConnectedTimeout;
    auto_disconnect_time_left_text_->SetLabelText(
        TimeDeltaToHumanReadableString(kConnectedTimeout));
  } else {
    UpdateText("Failed");
    UpdateGauge(0);
    SetTitle("Failed to connect to lobby");
    button_->SetLabelText("Close");
    lobby_connector_.reset();
  }

  wxBell();
}

void WxLobbyConnectDialog::OnActionButtonClick(wxCommandEvent&) {
  lobby_connector_.reset();
  UpdateText("Cancelled");
  UpdateGauge(0);
  SetTitle("Connection cancelled");
  if (as_modal_) {
    EndModal(wxID_CANCEL);
  } else {
    Destroy();
  }
}

void WxLobbyConnectDialog::OnAutoDisconnectChanged(wxCommandEvent&) {
  if (auto_disconnect_checkbox_->IsChecked()) {
    auto_disconnect_time_ = base::Time::Now() + kConnectedTimeout;
    auto_disconnect_time_left_text_->SetLabelText(
        TimeDeltaToHumanReadableString(kConnectedTimeout));
  } else {
    auto_disconnect_time_ = std::nullopt;
    auto_disconnect_time_left_text_->SetLabelText("-:--");
  }
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

void WxLobbyConnectDialog::AutoCloseOnDoneTimeout() {
  if (as_modal_) {
    EndModal(wxID_OK);
  } else {
    Destroy();
  }

  wxBell();
}

}  // namespace ui::wx
