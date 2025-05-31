#pragma once

#include "base/callback.h"

#include "models/auth.h"

namespace engine::backend {

class AuthBackend {
 public:
  virtual ~AuthBackend() = default;

  virtual void Authenticate(
      base::OnceCallback<void(model::AuthResult)> on_done_callback) = 0;
};

}  // namespace engine::backend
