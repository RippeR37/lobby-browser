#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/net/resource_response.h"
#include "base/threading/thread.h"
#include "base/time/time_ticks.h"
#include "engine/backends/result.h"

namespace engine::backend {

struct Quake3ServerResult {
  struct Member {
    int score;
    int ping;
    std::string name;
  };

  std::string address;
  std::string hostname;
  std::string map;
  int players = 0;
  int max_players = 0;
  std::string game_type;
  int ping = 0;
  std::map<std::string, std::string> metadata;
  std::vector<Member> members;
};

struct Quake3MasterSearchResponse {
  Result result;
  std::vector<Quake3ServerResult> servers;
};

class Quake3MasterBackend {
 public:
  static std::unique_ptr<Quake3MasterBackend> Create();

  Quake3MasterBackend();
  virtual ~Quake3MasterBackend();

  virtual void SearchServers(
      const std::vector<std::string>& master_servers,
      base::OnceCallback<void(Quake3MasterSearchResponse)>
          on_done_callback) = 0;

  virtual void GetServerDetails(
      const std::string& server_address,
      base::OnceCallback<void(base::net::ResourceResponse)>
          on_done_callback) = 0;
};

}  // namespace engine::backend
