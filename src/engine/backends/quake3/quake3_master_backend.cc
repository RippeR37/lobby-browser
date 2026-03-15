#include "engine/backends/quake3/quake3_master_backend.h"

#include <set>
#include <sstream>

#include "base/logging.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time_ticks.h"

#include <winsock2.h>
#include <ws2tcpip.h>

namespace engine::backend {

namespace {
const auto kDetailTimeout = base::Seconds(3);
const auto kMasterSearchTimeout = base::Seconds(3);

std::string SockAddrToString(const sockaddr_in& addr) {
  char buf[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));
  return std::string(buf) + ":" + std::to_string(ntohs(addr.sin_port));
}

bool AreSockAddrEqual(const sockaddr_in& a, const sockaddr_in& b) {
  return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}

std::string MapGameType(const std::string& type) {
  if (type == "0")
    return "FFA";
  if (type == "1")
    return "1v1";
  if (type == "2")
    return "Single";
  if (type == "3")
    return "TDM";
  if (type == "4")
    return "CTF";
  if (type == "5")
    return "OneFlag CTF";
  if (type == "6")
    return "Overload";
  if (type == "7")
    return "Harvester";
  if (type == "8")
    return "Team FFA";
  return "Unknown";
}

// This function processess a valid Quake 3 string (e.g. player name) and strips
// it from unwanted parts (e.g. color specifiers).
std::string CleanQuake3String(const std::string& input) {
  std::string output;
  for (size_t i = 0; i < input.size(); ++i) {
    if (input[i] == '^' && i + 1 < input.size() && std::isalnum(input[i + 1])) {
      i++;
      continue;
    }
    unsigned char c = static_cast<unsigned char>(input[i]);
    if (!std::isprint(c))
      output += ' ';
    else
      output += input[i];
  }
  return output;
}
}  // namespace

//
// Quake3MasterBackendImpl
//

class Quake3MasterBackendImpl : public Quake3MasterBackend {
 public:
  Quake3MasterBackendImpl();
  ~Quake3MasterBackendImpl() override;

  void SearchServers(const std::vector<std::string>& master_servers,
                     base::OnceCallback<void(Quake3MasterSearchResponse)>
                         on_done_callback) override;

  void GetServerDetails(const std::string& server_address,
                        base::OnceCallback<void(base::net::ResourceResponse)>
                            on_done_callback) override;

 private:
  struct PendingRequest {
    std::vector<base::OnceCallback<void(base::net::ResourceResponse)>>
        callbacks;
    base::TimeTicks start_time;
    sockaddr_in addr;
  };

  struct ActiveRefresh {
    base::OnceCallback<void(Quake3MasterSearchResponse)> callback;
    base::TimeTicks start_time;
    std::set<std::string> seen_addresses;
    Quake3MasterSearchResponse response;

    struct MasterState {
      sockaddr_in addr;
      bool got_eot = false;
    };
    std::vector<MasterState> masters;
    bool master_search_finished = false;
    int pending_details_count = 0;
  };

  void InitializeOnWorker();
  void ShutdownOnWorker();
  void SearchServersOnWorker(
      const std::vector<std::string> master_servers,
      base::OnceCallback<void(Quake3MasterSearchResponse)> callback);
  void SendDetailsRequestOnWorker(
      std::string address,
      base::OnceCallback<void(base::net::ResourceResponse)> callback);
  void PollOnWorker();
  void DispatchPacketOnWorker(const sockaddr_in& from,
                              const char* buffer,
                              int received);
  void FinalizeRefreshIfReadyOnWorker(bool force = false);

  void OnInternalDetailResponse(std::string address,
                                base::net::ResourceResponse response);

  base::Thread worker_thread_;
  SOCKET sock_ = INVALID_SOCKET;

  // Maps address string "IP:Port" to pending request
  std::map<std::string, PendingRequest> pending_requests_;
  std::unique_ptr<ActiveRefresh> active_refresh_;

  base::WeakPtr<Quake3MasterBackendImpl> weak_this_;
  base::WeakPtrFactory<Quake3MasterBackendImpl> weak_factory_;
};

// static
std::unique_ptr<Quake3MasterBackend> Quake3MasterBackend::Create() {
  return std::make_unique<Quake3MasterBackendImpl>();
}

Quake3MasterBackend::Quake3MasterBackend() = default;
Quake3MasterBackend::~Quake3MasterBackend() = default;

Quake3MasterBackendImpl::Quake3MasterBackendImpl() : weak_factory_(this) {
  weak_this_ = weak_factory_.GetWeakPtr();
  worker_thread_.Start();
  worker_thread_.TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&Quake3MasterBackendImpl::InitializeOnWorker, weak_this_));
}

Quake3MasterBackendImpl::~Quake3MasterBackendImpl() {
  worker_thread_.Stop(
      FROM_HERE,
      base::BindOnce(&Quake3MasterBackendImpl::ShutdownOnWorker, weak_this_));
}

void Quake3MasterBackendImpl::InitializeOnWorker() {
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    LOG(ERROR) << "WSAStartup failed in Quake3MasterBackendImpl";
    return;
  }

  sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock_ == INVALID_SOCKET) {
    LOG(ERROR) << "Failed to create UDP socket: " << WSAGetLastError();
    return;
  }

  u_long mode = 1;
  ioctlsocket(sock_, FIONBIO, &mode);

  PollOnWorker();
}

void Quake3MasterBackendImpl::ShutdownOnWorker() {
  if (sock_ != INVALID_SOCKET) {
    closesocket(sock_);
    sock_ = INVALID_SOCKET;
  }
  WSACleanup();
}

void Quake3MasterBackendImpl::SearchServers(
    const std::vector<std::string>& master_servers,
    base::OnceCallback<void(Quake3MasterSearchResponse)> on_done_callback) {
  worker_thread_.TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&Quake3MasterBackendImpl::SearchServersOnWorker,
                     weak_this_, master_servers, std::move(on_done_callback)));
}

void Quake3MasterBackendImpl::SearchServersOnWorker(
    const std::vector<std::string> master_servers,
    base::OnceCallback<void(Quake3MasterSearchResponse)> callback) {
  if (active_refresh_) {
    const auto kForce = true;
    FinalizeRefreshIfReadyOnWorker(kForce);
  }

  active_refresh_ = std::make_unique<ActiveRefresh>();
  active_refresh_->callback = std::move(callback);
  active_refresh_->start_time = base::TimeTicks::Now();
  active_refresh_->response.result.status = Result::Status::kOk;

  // Drain any stale packets from the socket.
  {
    char trash[4096];
    int from_len = sizeof(sockaddr_in);
    sockaddr_in from{};
    while (recvfrom(sock_, trash, sizeof(trash), 0, (sockaddr*)&from,
                    &from_len) > 0) {
    };
  }

  for (const auto& master : master_servers) {
    size_t colon_pos = master.find(':');
    std::string host =
        (colon_pos != std::string::npos) ? master.substr(0, colon_pos) : master;
    std::string port_str = (colon_pos != std::string::npos)
                               ? master.substr(colon_pos + 1)
                               : "27950";

    struct addrinfo hints{}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0) {
      LOG(WARNING) << "Failed to resolve master server: " << host;
      continue;
    }

    const char request[] = "\xff\xff\xff\xffgetservers 68";
    sendto(sock_, request, sizeof(request) - 1, 0, res->ai_addr,
           (int)res->ai_addrlen);

    ActiveRefresh::MasterState state;
    state.addr = *(sockaddr_in*)res->ai_addr;
    active_refresh_->masters.push_back(state);

    freeaddrinfo(res);
  }
}

void Quake3MasterBackendImpl::GetServerDetails(
    const std::string& server_address,
    base::OnceCallback<void(base::net::ResourceResponse)> on_done_callback) {
  worker_thread_.TaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&Quake3MasterBackendImpl::SendDetailsRequestOnWorker,
                     weak_this_, server_address, std::move(on_done_callback)));
}

void Quake3MasterBackendImpl::SendDetailsRequestOnWorker(
    std::string address,
    base::OnceCallback<void(base::net::ResourceResponse)> callback) {
  auto it = pending_requests_.find(address);
  if (it != pending_requests_.end()) {
    it->second.callbacks.push_back(std::move(callback));
    return;
  }

  size_t colon_pos = address.find(':');
  std::string host =
      (colon_pos != std::string::npos) ? address.substr(0, colon_pos) : address;
  uint16_t port = (colon_pos != std::string::npos)
                      ? (uint16_t)std::stoi(address.substr(colon_pos + 1))
                      : 27960;

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

  const char request[] = "\xff\xff\xff\xffgetstatus";
  sendto(sock_, request, sizeof(request) - 1, 0, (sockaddr*)&addr,
         sizeof(addr));

  PendingRequest pending;
  pending.callbacks.push_back(std::move(callback));
  pending.start_time = base::TimeTicks::Now();
  pending.addr = addr;

  pending_requests_[address] = std::move(pending);
}

void Quake3MasterBackendImpl::PollOnWorker() {
  if (sock_ == INVALID_SOCKET)
    return;

  char buffer[16384];
  sockaddr_in from{};
  int from_len = sizeof(from);
  int received;

  while ((received = recvfrom(sock_, buffer, sizeof(buffer), 0,
                              (sockaddr*)&from, &from_len)) > 0) {
    DispatchPacketOnWorker(from, buffer, received);
  }

  auto now = base::TimeTicks::Now();

  if (active_refresh_ && !active_refresh_->master_search_finished &&
      now - active_refresh_->start_time > kMasterSearchTimeout) {
    active_refresh_->master_search_finished = true;
    FinalizeRefreshIfReadyOnWorker();
  }

  for (auto it = pending_requests_.begin(); it != pending_requests_.end();) {
    if (now - it->second.start_time > kDetailTimeout) {
      auto callbacks = std::move(it->second.callbacks);
      for (auto& cb : callbacks) {
        base::net::ResourceResponse response;
        response.result = base::net::Result::kError;
        std::move(cb).Run(std::move(response));
      }
      it = pending_requests_.erase(it);
    } else {
      ++it;
    }
  }

  base::TimeDelta interval = (active_refresh_ || !pending_requests_.empty())
                                 ? base::Milliseconds(1)
                                 : base::Milliseconds(10);

  worker_thread_.TaskRunner()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&Quake3MasterBackendImpl::PollOnWorker, weak_this_),
      interval);
}

void Quake3MasterBackendImpl::DispatchPacketOnWorker(const sockaddr_in& from,
                                                     const char* buffer,
                                                     int received) {
  if (active_refresh_) {
    for (auto& master : active_refresh_->masters) {
      if (AreSockAddrEqual(from, master.addr)) {
        std::string data(buffer, received);
        size_t pos = data.find("getserversResponse");
        if (pos != std::string::npos) {
          pos += 18;
          while (pos < (size_t)received) {
            if (buffer[pos] == '\\' && pos + 7 <= (size_t)received) {
              std::string host =
                  std::to_string((unsigned char)buffer[pos + 1]) + "." +
                  std::to_string((unsigned char)buffer[pos + 2]) + "." +
                  std::to_string((unsigned char)buffer[pos + 3]) + "." +
                  std::to_string((unsigned char)buffer[pos + 4]);
              uint16_t port = ((unsigned char)buffer[pos + 5] << 8) |
                              (unsigned char)buffer[pos + 6];

              std::string addr_str = host + ":" + std::to_string(port);
              if (active_refresh_->seen_addresses.insert(addr_str).second) {
                // NEW SERVER FOUND: Start detail query immediately!
                active_refresh_->pending_details_count++;
                SendDetailsRequestOnWorker(
                    addr_str,
                    base::BindOnce(
                        &Quake3MasterBackendImpl::OnInternalDetailResponse,
                        weak_this_, addr_str));
              }
              pos += 7;
            } else if (buffer[pos] == 'E' && pos + 2 < (size_t)received &&
                       buffer[pos + 1] == 'O' && buffer[pos + 2] == 'T') {
              master.got_eot = true;
              break;
            } else {
              pos++;
            }
          }

          bool all_finished = true;
          for (const auto& m : active_refresh_->masters) {
            if (!m.got_eot) {
              all_finished = false;
              break;
            }
          }
          if (all_finished) {
            active_refresh_->master_search_finished = true;
            FinalizeRefreshIfReadyOnWorker();
          }
        }
        return;
      }
    }
  }

  std::string addr_str = SockAddrToString(from);
  auto it = pending_requests_.find(addr_str);
  if (it != pending_requests_.end()) {
    base::TimeDelta timing = base::TimeTicks::Now() - it->second.start_time;
    auto callbacks = std::move(it->second.callbacks);
    pending_requests_.erase(it);

    for (auto& cb : callbacks) {
      base::net::ResourceResponse response;
      response.result = base::net::Result::kOk;
      response.data.assign(buffer, buffer + received);
      response.timing_connect = timing;
      response.timing_queue = base::Seconds(0);
      std::move(cb).Run(std::move(response));
    }
  }
}

void Quake3MasterBackendImpl::OnInternalDetailResponse(
    std::string address,
    base::net::ResourceResponse response) {
  if (!active_refresh_)
    return;

  if (response.result == base::net::Result::kOk) {
    std::string data = response.DataAsString();
    size_t header_pos = data.find("statusResponse\n");
    if (header_pos != std::string::npos) {
      std::string content = data.substr(header_pos + 15);
      size_t player_list_pos = content.find('\n');

      std::string cvar_block = content.substr(0, player_list_pos);
      if (!cvar_block.empty() && cvar_block[0] == '\\')
        cvar_block = cvar_block.substr(1);

      std::map<std::string, std::string> cvars;
      std::stringstream ss(cvar_block);
      std::string key, val;
      while (std::getline(ss, key, '\\') && std::getline(ss, val, '\\')) {
        if (!key.empty())
          cvars[key] = val;
      }

      int player_count = 0;
      std::vector<Quake3ServerResult::Member> members;
      if (player_list_pos != std::string::npos) {
        std::string player_list = content.substr(player_list_pos + 1);
        std::stringstream pss(player_list);
        std::string player_line;
        while (std::getline(pss, player_line)) {
          if (player_line.empty())
            continue;

          std::stringstream lss(player_line);
          int score, ping;
          std::string name;
          if (lss >> score >> ping) {
            std::getline(lss, name);
            // Remove quotes and leading spaces
            size_t first = name.find_first_not_of(" ");
            if (first != std::string::npos)
              name = name.substr(first);
            if (name.size() >= 2 && name.front() == '"' && name.back() == '"') {
              name = name.substr(1, name.size() - 2);
            }

            members.push_back({score, ping, CleanQuake3String(name)});
            player_count++;
          }
        }
      }

      Quake3ServerResult result;
      result.address = address;
      result.hostname = CleanQuake3String(cvars["sv_hostname"]);
      if (result.hostname.empty())
        result.hostname = cvars["sv_hostname"];
      if (result.hostname.empty())
        result.hostname = "Unnamed Server";

      result.map = CleanQuake3String(cvars["mapname"]);
      if (result.map.empty())
        result.map = cvars["mapname"];
      if (result.map.empty())
        result.map = "Unknown Map";

      try {
        result.max_players = std::stoi(cvars["sv_maxclients"]);
      } catch (...) {
      }
      result.players = player_count;
      result.game_type = MapGameType(cvars["g_gametype"]);
      result.ping = (int)response.timing_connect.InMilliseconds();
      result.metadata = std::move(cvars);
      result.members = std::move(members);

      active_refresh_->response.servers.push_back(std::move(result));
    }
  }

  active_refresh_->pending_details_count--;
  FinalizeRefreshIfReadyOnWorker();
}

void Quake3MasterBackendImpl::FinalizeRefreshIfReadyOnWorker(bool force) {
  if (!active_refresh_)
    return;
  if ((active_refresh_->master_search_finished &&
       active_refresh_->pending_details_count <= 0) ||
      force) {
    std::move(active_refresh_->callback)
        .Run(std::move(active_refresh_->response));
    active_refresh_.reset();
  }
}

}  // namespace engine::backend
