#include "models/serialize/auth.h"

namespace model {

void to_json(nlohmann::json& out, const AuthResult& obj) {
  out = {
      {"success", obj.success},       {"error", obj.error},
      {"user_name", obj.user_name},   {"user_id", obj.user_id},
      {"auth_token", obj.auth_token},
  };
}

void from_json(const nlohmann::json& in, AuthResult& obj) {
  obj.success = in.value("success", false);
  obj.error = in.value("error", "");
  obj.user_name = in.value("user_name", "");
  obj.user_id = in.value("user_id", "");
  obj.auth_token = in.value("auth_token", "");
}

}  // namespace model
