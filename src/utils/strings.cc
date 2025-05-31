#include "utils/strings.h"

#include <algorithm>

namespace util {

std::vector<std::string_view> SplitString(std::string_view input,
                                          char delimiter) {
  std::vector<std::string_view> result;
  size_t start = 0;

  while (start < input.size()) {
    size_t end = input.find(delimiter, start);
    if (end == std::string_view::npos) {
      result.emplace_back(input.substr(start));
      break;
    }
    result.emplace_back(input.substr(start, end - start));
    start = end + 1;
  }

  return result;
}

void ReplaceAll(std::string& result,
                const std::string& pattern,
                const std::string& replacement) {
  size_t pos = 0;
  while ((pos = result.find(pattern, pos)) != std::string::npos) {
    result.replace(pos, pattern.length(), replacement);
    pos += replacement.length();
  }
}

std::string ToLower(const std::string& input) {
  std::string result = input;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return result;
}

}  // namespace util
