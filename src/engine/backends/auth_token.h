#pragma once

#include <optional>
#include <utility>

#include "base/time/time.h"
#include "base/time/time_delta.h"

namespace engine::backend {

template <typename TokenType>
class AuthToken {
 public:
  AuthToken() : token_(std::nullopt), expire_(base::Time{}) {}
  AuthToken(TokenType token, base::TimeDelta ttl) : token_(), expire_() {
    token_ = std::move(token);
    if (!ttl.IsZero()) {
      expire_ = base::Time::Now() + ttl;
    }
  }

  bool IsValid() const {
    if (!token_) {
      return false;
    }
    return (expire_ == base::Time{}) || (base::Time::Now() < expire_);
  }

  std::optional<TokenType> GetToken() {
    if (IsValid()) {
      return token_;
    }
    return std::nullopt;
  }

 private:
  std::optional<TokenType> token_;
  base::Time expire_;
};

}  // namespace engine::backend
