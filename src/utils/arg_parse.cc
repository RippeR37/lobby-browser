#include "utils/arg_parse.h"

namespace util {

std::optional<std::string_view> ParseArg(
    int argc,
    // NOLINTBEGIN(modernize-avoid-c-arrays)
    char* argv[],
    // NOLINTEND(modernize-avoid-c-arrays)
    std::string_view arg_name) {
  for (int idx = 0; idx < argc; ++idx) {
    const auto arg_value = std::string_view{argv[idx]};
    if (arg_value.find(arg_name) == 0) {
      return arg_value.substr(arg_name.length());
    }
  }
  return std::nullopt;
}

}  // namespace util
