#pragma once

#include <string>
#include <vector>

namespace model {

enum class GameConfigOptionType {
  kString,
  kFilePath,
  kDirectoryPath,
  kEditableList,
};

struct GameConfigOption {
  std::string key;
  std::string label;
  std::string description;
  GameConfigOptionType type;
  std::string value; // For scalar types

  // For kEditableList
  struct ListColumn {
      std::string name;
      int width;
  };
  std::vector<ListColumn> list_columns;
  struct ListItem {
      std::string id; // Usually address or unique key
      std::vector<std::string> fields;
  };
  std::vector<ListItem> list_items;
};

struct GameConfigSection {
  std::string name;
  std::vector<GameConfigOption> options;
};

struct GameConfigDescriptor {
  std::vector<GameConfigSection> sections;

  bool empty() const { return sections.empty(); }
};

}  // namespace model
