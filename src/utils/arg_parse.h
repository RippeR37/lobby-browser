#pragma once

#include <optional>
#include <string_view>

namespace util {

std::optional<std::string_view> ParseArg(
    int argc,
    // NOLINTBEGIN(modernize-avoid-c-arrays)
    char* argv[],
    // NOLINTEND(modernize-avoid-c-arrays)
    std::string_view arg_name);

};  // namespace util
