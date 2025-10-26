#include "engine/backends/eos/eos_lobby_connector.h"

#include <algorithm>
#include <random>

#include "base/bind_post_task.h"
#include "base/memory/weak_ptr.h"
#include "ixwebsocket/IXWebSocket.h"

#include "engine/backends/eos/eos_data.h"
#include "engine/backends/eos/eos_data_serialize.h"

namespace engine::backend {

namespace {
const auto kJoinFullLobbyRetryInterval = base::Seconds(5);

std::string GetLobbiesConnectUrl(std::string product_id) {
  return "wss://api.epicgames.dev/lobby/v1/" + product_id + "/lobbies/connect";
}

std::string GenerateRandomID(size_t length = 22) {
  // NOLINTBEGIN
  static const char charset[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

  static std::mt19937 rng(std::random_device{}());
  static std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);

  std::string id;
  id.reserve(length);
  for (size_t i = 0; i < length; ++i) {
    id += charset[dist(rng)];
  }
  return id;
}
}  // namespace

EosLobbyConnector::EosLobbyConnector(
    std::string deployment_id,
    AuthToken<std::string> access_token,
    std::string lobby_id,
    base::RepeatingCallback<void(std::string)> status_update_cb,
    base::RepeatingCallback<void(int)> progress_update_cb,
    base::RepeatingCallback<void(LobbyStateUpdate)> state_update_cb,
    base::OnceCallback<void(bool)> on_done_callback) {
  worker_thread_.Start();
  client_ = std::make_unique<EosLobbyConnector::ClientImpl>(
      worker_thread_.TaskRunner(), std::move(deployment_id),
      std::move(access_token), std::move(lobby_id), std::move(status_update_cb),
      std::move(progress_update_cb), std::move(state_update_cb),
      std::move(on_done_callback));
}

EosLobbyConnector::~EosLobbyConnector() {
  worker_thread_.TaskRunner()->DeleteSoon(FROM_HERE, std::move(client_));
  worker_thread_.Stop();
}

//
// ClientImpl
//

class EosLobbyConnector::ClientImpl {
 public:
  // Created on _any_ thread, will be destroyed on `task_runner` and run all
  // member functions on it as well.
  ClientImpl(std::shared_ptr<base::SequencedTaskRunner> task_runner,
             std::string deployment_id,
             AuthToken<std::string> access_token,
             std::string lobby_id,
             base::RepeatingCallback<void(std::string)> status_update_cb,
             base::RepeatingCallback<void(int)> progress_update_cb,
             base::RepeatingCallback<void(LobbyStateUpdate)> state_update_cb,
             base::OnceCallback<void(bool)> on_done_callback);
  ~ClientImpl();

 private:
  struct LobbyState {
    std::map<std::string, eos::WsMemberData> members;
    std::string owner_id;
    std::string game_mode;
    std::string map;
    std::string state;
    int max_players;
  };

  void InitializeOnTaskRunner(std::string deployment_id);
  void TryJoinOnTaskRunner();
  void ReportProgressOnTaskRunner(bool waiting);
  void ReportDoneOnTaskRunner(bool success);

  // WebSocket callbacks
  void OnWebSocketOpenOnTaskRunner();
  void OnWebSocketMessageOnTaskRunner(std::string message, bool binary);
  void OnWebSocketCloseOnTaskRunner(int code, std::string reason);
  void OnWebSocketErrorOnTaskRunner(std::string reason);

  void TrackLobbyState(std::string msg_name, nlohmann::json msg);
  void SendLobbyStateUpdate() const;

  std::shared_ptr<base::SequencedTaskRunner> task_runner_;
  AuthToken<std::string> access_token_;
  std::string lobby_id_;
  base::RepeatingCallback<void(std::string)> status_update_cb_;
  base::RepeatingCallback<void(int)> progress_update_cb_;
  base::RepeatingCallback<void(LobbyStateUpdate)> state_update_cb_;
  base::OnceCallback<void(bool)> on_done_callback_;
  std::unique_ptr<ix::WebSocket> ws_;
  size_t join_counter_ = 0;

  LobbyState lobby_state_;

  base::WeakPtr<ClientImpl> weak_this_;
  base::WeakPtrFactory<ClientImpl> weak_factory_;
};

EosLobbyConnector::ClientImpl::ClientImpl(
    std::shared_ptr<base::SequencedTaskRunner> task_runner,
    std::string deployment_id,
    AuthToken<std::string> access_token,
    std::string lobby_id,
    base::RepeatingCallback<void(std::string)> status_update_cb,
    base::RepeatingCallback<void(int)> progress_update_cb,
    base::RepeatingCallback<void(LobbyStateUpdate)> state_update_cb,
    base::OnceCallback<void(bool)> on_done_callback)
    : task_runner_(std::move(task_runner)),
      access_token_(std::move(access_token)),
      lobby_id_(std::move(lobby_id)),
      status_update_cb_(
          base::BindToCurrentSequence(std::move(status_update_cb), FROM_HERE)),
      progress_update_cb_(
          base::BindToCurrentSequence(std::move(progress_update_cb),
                                      FROM_HERE)),
      state_update_cb_(
          base::BindToCurrentSequence(std::move(state_update_cb), FROM_HERE)),
      on_done_callback_(
          base::BindToCurrentSequence(std::move(on_done_callback), FROM_HERE)),
      weak_factory_(this) {
  DCHECK(!task_runner_->RunsTasksInCurrentSequence());
  weak_this_ = weak_factory_.GetWeakPtr();

  status_update_cb_.Run("Connecting");
  progress_update_cb_.Run(0);

  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&ClientImpl::InitializeOnTaskRunner, weak_this_,
                                deployment_id));
}

EosLobbyConnector::ClientImpl::~ClientImpl() {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  if (ws_) {
    ws_->stop();
  }
}

void EosLobbyConnector::ClientImpl::InitializeOnTaskRunner(
    std::string deployment_id) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  ws_ = std::make_unique<ix::WebSocket>();
  ws_->setUrl(GetLobbiesConnectUrl(deployment_id));
  ws_->setExtraHeaders({
      {"Authorization", "Bearer " + access_token_.GetToken().value_or("")},
  });
  ws_->setOnMessageCallback([task_runner = task_runner_,
                             weak_this = weak_this_](
                                const ix::WebSocketMessagePtr& msg) {
    switch (msg->type) {
      case ix::WebSocketMessageType::Open:
        task_runner->PostTask(
            FROM_HERE, base::BindOnce(&ClientImpl::OnWebSocketOpenOnTaskRunner,
                                      weak_this));
        break;
        break;

      case ix::WebSocketMessageType::Message:
        task_runner->PostTask(
            FROM_HERE,
            base::BindOnce(&ClientImpl::OnWebSocketMessageOnTaskRunner,
                           weak_this, msg->str, msg->binary));
        break;

      case ix::WebSocketMessageType::Close:
        task_runner->PostTask(
            FROM_HERE,
            base::BindOnce(&ClientImpl::OnWebSocketCloseOnTaskRunner, weak_this,
                           msg->closeInfo.code, msg->closeInfo.reason));

      case ix::WebSocketMessageType::Error:
        task_runner->PostTask(
            FROM_HERE, base::BindOnce(&ClientImpl::OnWebSocketErrorOnTaskRunner,
                                      weak_this, msg->errorInfo.reason));
        break;

      case ix::WebSocketMessageType::Ping:
      case ix::WebSocketMessageType::Pong:
      case ix::WebSocketMessageType::Fragment:
        // ignore these
        break;
    }
  });

  ws_->start();
}

void EosLobbyConnector::ClientImpl::TryJoinOnTaskRunner() {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  join_counter_++;

  ReportProgressOnTaskRunner(false);

  eos::WsJoinRequest join_msg{GenerateRandomID(), lobby_id_, true};
  nlohmann::json json_msg = join_msg;
  ws_->sendText(json_msg.dump());
}

void EosLobbyConnector::ClientImpl::ReportProgressOnTaskRunner(bool waiting) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  if (!on_done_callback_) {
    return;
  }

  const std::string details = waiting ? "\nWaiting" : "";

  status_update_cb_.Run("Joining (#" + std::to_string(join_counter_) + ")" +
                        details);
  progress_update_cb_.Run(10 + std::min<size_t>(5 * join_counter_, 70));
}

void EosLobbyConnector::ClientImpl::ReportDoneOnTaskRunner(bool success) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  if (on_done_callback_) {
    std::move(on_done_callback_).Run(success);
  }
}

void EosLobbyConnector::ClientImpl::OnWebSocketOpenOnTaskRunner() {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  TryJoinOnTaskRunner();
}

void EosLobbyConnector::ClientImpl::OnWebSocketMessageOnTaskRunner(
    std::string message,
    bool binary) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  if (binary) {
    LOG(ERROR) << __FUNCTION__ << "() unexpected binary data, ignoring...";
    return;
  }

  nlohmann::json json_msg = nlohmann::json::parse(message);

  if (!json_msg.contains("name") || !json_msg["name"].is_string()) {
    LOG(ERROR) << __FUNCTION__
               << "() malformed response doesn't contain field 'name'";
    return;
  }

  const std::string msg_name = json_msg["name"];
  if (msg_name == "error") {
    eos::WsErrorResponse error_msg = json_msg;
    if (error_msg.status_code == 400 &&
        error_msg.error_code.find("too_many_players") != std::string::npos) {
      // This is `too_many_players` error, let's retry in 5 seconds
      ReportProgressOnTaskRunner(true);
      task_runner_->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&ClientImpl::TryJoinOnTaskRunner, weak_this_),
          kJoinFullLobbyRetryInterval);
      return;
    }

    if (error_msg.status_code == 404) {
      ReportDoneOnTaskRunner(false);
      status_update_cb_.Run("Not found");
      progress_update_cb_.Run(0);

      // This lobby doesn't exist anymore, stop
      if (ws_->getReadyState() == ix::ReadyState::Open) {
        ws_->close();
      }
      return;
    }

    LOG(ERROR) << __FUNCTION__ << "() unknown error " << error_msg.status_code
               << " (" << error_msg.error_code
               << "): " << error_msg.error_message
               << " | full response: " << message;

    ReportDoneOnTaskRunner(false);
    status_update_cb_.Run("Error (" + std::to_string(error_msg.status_code) +
                          ")");

    return;
  }

  if (msg_name == "lobbyinfo") {
    ReportDoneOnTaskRunner(true);
  }

  TrackLobbyState(msg_name, json_msg);
}

void EosLobbyConnector::ClientImpl::OnWebSocketCloseOnTaskRunner(
    int code,
    std::string reason) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  LOG(INFO) << __FUNCTION__ << "(code: " << code << ", reason: " << reason
            << ")";
}

void EosLobbyConnector::ClientImpl::OnWebSocketErrorOnTaskRunner(
    std::string reason) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  LOG(ERROR) << __FUNCTION__ << "(reason: " << reason << ")";
}

void EosLobbyConnector::ClientImpl::TrackLobbyState(std::string msg_name,
                                                    nlohmann::json json_msg) {
  try {
    if (msg_name == "lobbyinfo") {
      eos::WsLobbyInfoResponse msg = json_msg;
      lobby_state_.max_players = msg.members.size() + msg.open_public_players;
      lobby_state_.owner_id = msg.owner_id;
      lobby_state_.game_mode = msg.game_mode;
      lobby_state_.map = msg.map;
      lobby_state_.members.clear();
      for (const auto& member : msg.members) {
        lobby_state_.members[member.id] = member;
      }
      lobby_state_.state = msg.state;

    } else if (msg_name == "lobbydatachange") {
      eos::WsLobbyDataChangeMessage msg = json_msg;
      lobby_state_.game_mode = msg.game_mode;
      lobby_state_.map = msg.map;
      lobby_state_.state = msg.state;

      bool updated_owner = false;
      for (const auto& member : lobby_state_.members) {
        if (member.second.display_name == msg.owner_name) {
          lobby_state_.owner_id = member.first;
          updated_owner = true;
        }
      }
      if (!updated_owner) {
        lobby_state_.owner_id = msg.owner_name;
      }

    } else if (msg_name == "memberjoined") {
      eos::WsMemberJoinedMessage msg = json_msg;
      if (lobby_state_.members.count(msg.player_uid) == 0) {
        lobby_state_.members[msg.player_uid] =
            eos::WsMemberData{msg.player_uid, "<waiting>"};
      }

    } else if (msg_name == "memberleft") {
      eos::WsMemberLeftMessage msg = json_msg;
      lobby_state_.members.erase(msg.player_uid);

    } else if (msg_name == "memberdatachange") {
      eos::WsMemberDataChangeMessage msg = json_msg;
      auto member_it = lobby_state_.members.find(msg.player_uid);
      if (member_it != lobby_state_.members.end()) {
        if (!msg.display_name.empty()) {
          member_it->second.display_name = msg.display_name;
        }
      }

    } else {
      // Unknown message
      return;
    }

    SendLobbyStateUpdate();
  } catch (const std::exception& e) {
    LOG(ERROR) << __FUNCTION__ << "() failed with error: " << e.what();
  }
}

void EosLobbyConnector::ClientImpl::SendLobbyStateUpdate() const {
  if (!state_update_cb_) {
    return;
  }

  model::LobbyConnector::LobbyStateUpdate update;

  auto owner_it = lobby_state_.members.find(lobby_state_.owner_id);
  if (owner_it != lobby_state_.members.end()) {
    update.owner = owner_it->second.display_name;
  } else {
    update.owner = lobby_state_.owner_id.substr(0, 16);
  }

  update.players = std::to_string(lobby_state_.members.size()) + "/" +
                   std::to_string(lobby_state_.max_players);

  update.game_mode = lobby_state_.game_mode;
  update.map = lobby_state_.map;
  update.state = lobby_state_.state;

  state_update_cb_.Run(std::move(update));
}

}  // namespace engine::backend
