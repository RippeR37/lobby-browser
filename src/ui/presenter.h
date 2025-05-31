#pragma once

#include "engine/presenter.h"

namespace ui {

class Presenter : public engine::Presenter {
 public:
  Presenter();
  ~Presenter() override;

  virtual void ShowMainWindow() = 0;
};

};  // namespace ui
