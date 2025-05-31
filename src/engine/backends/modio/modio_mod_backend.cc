#include "engine/backends/modio/modio_mod_backend.h"

#include "base/barrier_callback.h"
#include "base/net/simple_url_loader.h"

#include "engine/backends/modio/modio_data_serialize.h"

namespace engine::backend {

namespace {
std::string GetModsUrlForPage(const std::string& server_url,
                              int64_t game_id,
                              const std::string& api_key,
                              int page) {
  return server_url + "/v1/games/" + std::to_string(game_id) +
         "/mods?api_key=" + api_key +
         "&visible=1&_limit=100&_offset=" + std::to_string(page * 100) +
         "&tags=Windows&name-lk=**&_sort=-date_updated";
}
}  // namespace

ModIoModBackend::ModIoModBackend(std::string server_url,
                                 int64_t game_id,
                                 std::string api_key)
    : server_url_(std::move(server_url)),
      game_id_(game_id),
      api_key_(std::move(api_key)),
      weak_factory_(this) {
  weak_this_ = weak_factory_.GetWeakPtr();
}

ModIoModBackend::~ModIoModBackend() = default;

bool ModIoModBackend::IsReady() const {
  return !on_ready_callback_;
}

void ModIoModBackend::FetchMods(base::OnceClosure on_ready_callback) {
  on_ready_callback_ = std::move(on_ready_callback);

  const auto kMaxResponseSize = 15 * 1024 * 1024;  // 15 MB
  base::net::SimpleUrlLoader::DownloadLimited(
      base::net::ResourceRequest{
          GetModsUrlForPage(server_url_, game_id_, api_key_, 0)}
          .WithTimeout(base::Seconds(10)),
      kMaxResponseSize,
      base::BindOnce(&ModIoModBackend::OnFetchModsInfo, weak_this_));
}

std::optional<modio::ModData> ModIoModBackend::GetModData(
    int64_t mod_id) const {
  if (auto mod_it = mods_.find(mod_id); mod_it != mods_.end()) {
    return mod_it->second;
  }
  return std::nullopt;
}

void ModIoModBackend::OnFetchModsInfo(base::net::ResourceResponse response) {
  if (response.result != base::net::Result::kOk) {
    LOG(ERROR) << "Failed to fetch mod listing, result: "
               << static_cast<int>(response.result);
    std::move(on_ready_callback_).Run();
    return;
  }

  int64_t page_count = 0;
  try {
    nlohmann::json json_response = nlohmann::json::parse(response.data);
    modio::ModListingPage page_data = json_response;

    page_count = ((page_data.result_total - 1) / 100) + 1;

    ProcessModsPageData(std::move(page_data));
  } catch (const std::exception& e) {
    LOG(WARNING) << "Failed to parse ModIo response: " << e.what();
    std::move(on_ready_callback_).Run();
    return;
  }

  if (page_count <= 1) {
    std::move(on_ready_callback_).Run();
    return;
  }

  // Fetch all the other pages here and call OnFetchModsPages when done
  auto chunk_response_callback =
      base::BarrierCallback<base::net::ResourceResponse>(
          page_count - 1,  // we got page 0 already
          base::BindOnce(&ModIoModBackend::OnFetchModsPages, weak_this_));

  // Start fetching the other pages
  for (int page_idx = 1; page_idx < page_count; ++page_idx) {
    const auto kMaxResponseSize = 15 * 1024 * 1024;  // 15 MB
    base::net::SimpleUrlLoader::DownloadLimited(
        base::net::ResourceRequest{
            GetModsUrlForPage(server_url_, game_id_, api_key_, page_idx)}
            .WithTimeout(base::Seconds(10)),
        kMaxResponseSize, chunk_response_callback);
  }
}

void ModIoModBackend::OnFetchModsPages(
    std::vector<base::net::ResourceResponse> responses) {
  for (auto& response : responses) {
    if (response.result != base::net::Result::kOk) {
      LOG(WARNING) << "Failed to fetch mod page, result: "
                   << static_cast<int>(response.result);
      continue;
    }

    try {
      nlohmann::json json_response = nlohmann::json::parse(response.data);
      modio::ModListingPage page_data = json_response;

      ProcessModsPageData(std::move(page_data));
    } catch (const std::exception& e) {
      LOG(WARNING) << "Failed to parse ModIo response: " << e.what();
    }
  }

  std::move(on_ready_callback_).Run();
}

void ModIoModBackend::ProcessModsPageData(modio::ModListingPage mods_page) {
  for (auto& mod : mods_page.data) {
    auto id = mod.id;
    mods_[id] = std::move(mod);
  }
}

}  // namespace engine::backend

//
