#pragma once

#include "base/memory/weak_ptr.h"
#include "wx/event.h"

#include "ui/event_handler.h"
#include "ui/presenter.h"
#include "ui/wx/wx_main_window.h"

namespace ui::wx {

class WxPresenter : public Presenter {
 public:
  WxPresenter(EventHandler* event_handler);
  ~WxPresenter() override;

  // engine::Presenter
  std::string GetPresenterName() const override;
  void Initialize(model::AppConfig app_config,
                  model::UiConfig ui_config,
                  std::vector<model::Game> game_models) override;
  void SetStatusText(std::string status_text) override;
  void ReportMessage(MessageType type,
                     std::string message,
                     std::string details) override;

  // ui::Presenter
  void ShowMainWindow() override;

 private:
  EventHandler* event_handler_;
  WxMainWindow* main_window_;

  base::WeakPtr<WxPresenter> weak_this_;
  base::WeakPtrFactory<WxPresenter> weak_factory_;
};

}  // namespace ui::wx
