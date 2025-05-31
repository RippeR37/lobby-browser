#pragma once

#include <vector>

namespace util {

template <typename T>
std::vector<std::vector<T>> ToChunks(const std::vector<T>& input,
                                     size_t chunk_size) {
  std::vector<std::vector<T>> chunks;
  for (std::size_t offset = 0; offset < input.size(); offset += chunk_size) {
    auto end = std::min(offset + chunk_size, input.size());
    chunks.emplace_back(input.begin() + offset, input.begin() + end);
  }
  return chunks;
}

}  // namespace util
