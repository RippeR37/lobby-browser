#pragma once

#include "models/config.h"

namespace engine::config {

class ConfigLoader {
 public:
  ConfigLoader();
  ~ConfigLoader();

  void LoadConfig();
  void SaveConfig();

  bool IsConfigLoaded() const;
  model::Config& GetConfig();
  const model::Config& GetConfig() const;

 private:
  bool config_loaded_;
  model::Config config_;
};

}  // namespace engine::config
