#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/net/resource_response.h"

#include "engine/backends/modio/modio_data.h"

namespace engine::backend {

class ModIoModBackend {
 public:
  ModIoModBackend(std::string server_url, int64_t game_id, std::string api_key);
  ~ModIoModBackend();

  bool IsReady() const;
  void FetchMods(base::OnceClosure on_ready_callback);
  std::optional<modio::ModData> GetModData(int64_t mod_id) const;

 private:
  void OnFetchModsInfo(base::net::ResourceResponse response);
  void OnFetchModsPages(std::vector<base::net::ResourceResponse> responses);
  void ProcessModsPageData(modio::ModListingPage mods_page);

  std::string server_url_;
  int64_t game_id_;
  std::string api_key_;

  base::OnceClosure on_ready_callback_;
  std::unordered_map<int64_t, modio::ModData> mods_;

  base::WeakPtr<ModIoModBackend> weak_this_;
  base::WeakPtrFactory<ModIoModBackend> weak_factory_;
};

}  // namespace engine::backend
