#include "engine/config/config_loader.h"

#include <fstream>

#include "base/logging.h"
#include "nlohmann/json.hpp"

#include "models/config.h"
#include "models/serialize/config.h"

namespace engine::config {

namespace {
const char* kConfigFilePath = "config.dat";
}

ConfigLoader::ConfigLoader() : config_loaded_(false) {}

ConfigLoader::~ConfigLoader() = default;

void ConfigLoader::LoadConfig() {
  std::ifstream config_file(kConfigFilePath);
  if (!config_file) {
    LOG(INFO) << "Config file not found";
    return;
  }

  try {
    nlohmann::json json_config = nlohmann::json::parse(config_file);
    config_ = json_config;
    config_loaded_ = true;
    LOG(INFO) << "Config loaded successfully";
  } catch (const std::exception& e) {
    LOG(ERROR) << "Failed to load config: " << e.what();
  }
}

void ConfigLoader::SaveConfig() {
  try {
    nlohmann::json json_config = config_;
    std::ofstream config_file(kConfigFilePath);
    config_file << std::setw(2) << json_config;
    LOG(INFO) << "Config saved successfully";
  } catch (const std::exception& e) {
    LOG(ERROR) << "Failed to save config: " << e.what();
  }
}

bool ConfigLoader::IsConfigLoaded() const {
  return config_loaded_;
}

model::Config& ConfigLoader::GetConfig() {
  return config_;
}

const model::Config& ConfigLoader::GetConfig() const {
  return config_;
}

}  // namespace engine::config
