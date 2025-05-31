#pragma once

#include "models/auth.h"

#include "nlohmann/json.hpp"

namespace model {

void to_json(nlohmann::json& out, const AuthResult& obj);
void from_json(const nlohmann::json& in, AuthResult& obj);

}  // namespace model
