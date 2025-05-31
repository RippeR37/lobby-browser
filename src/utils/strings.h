#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace util {

std::vector<std::string_view> SplitString(std::string_view input,
                                          char delimiter);
void ReplaceAll(std::string& result,
                const std::string& pattern,
                const std::string& replacement);

std::string ToLower(const std::string& input);

}  // namespace util
