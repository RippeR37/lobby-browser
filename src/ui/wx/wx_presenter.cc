#include "ui/wx/wx_presenter.h"

#include "wx/msw/darkmode.h"
#include "wx/pen.h"
#include "wx/richmsgdlg.h"
#include "wx/wx.h"

#include "ui/wx/wx_theme.h"

namespace ui::wx {

WxPresenter::WxPresenter(EventHandler* event_handler)
    : event_handler_(event_handler), weak_factory_(this) {
  weak_this_ = weak_factory_.GetWeakPtr();
}

WxPresenter::~WxPresenter() = default;

std::string WxPresenter::GetPresenterName() const {
  return "wx";
}

void WxPresenter::Initialize(model::AppConfig app_config,
                             model::UiConfig ui_config,
                             std::vector<model::Game> game_models) {
  wxInitAllImageHandlers();

  // Config parsing
  UiConfig ui_config_concrete;
  if (!ui_config.is_null()) {
    try {
      ui_config_concrete = ui_config;
    } catch (const std::exception& e) {
      LOG(ERROR) << __FUNCTION__
                 << "() failed to load game config: " << e.what();
    }
  }

  switch (ui_config_concrete.preferences.theme) {
    case UiTheme::Light:
      // don't do anything?
      break;

    case UiTheme::Dark:
      wxTheApp->MSWEnableDarkMode(wxApp::DarkMode_Always,
                                  new WxDarkModeSettings());
      break;

    case UiTheme::System:
      wxTheApp->MSWEnableDarkMode(wxApp::DarkMode_Auto,
                                  new WxDarkModeSettings());
      break;
  }

  auto theme_colors = GetThemeColors(ui_config_concrete.preferences.theme);

  main_window_ = new WxMainWindow(
      event_handler_,
      base::BindRepeating(&WxPresenter::ReportMessage, weak_this_),
      theme_colors);
  main_window_->Initialize(app_config, ui_config, std::move(game_models));
}

void WxPresenter::SetStatusText(std::string status_text) {
  main_window_->SetStatusText(std::move(status_text));
}

void WxPresenter::ReportMessage(MessageType type,
                                std::string message,
                                std::string details) {
  const auto icon_style = [type]() {
    switch (type) {
      case engine::Presenter::MessageType::kInfo:
        return wxICON_INFORMATION;

      case engine::Presenter::MessageType::kWarning:
        return wxICON_WARNING;

      case engine::Presenter::MessageType::kError:
      case engine::Presenter::MessageType::kFatalError:
        return wxICON_ERROR;
    }
  }();

  wxRichMessageDialog dialog{main_window_, message,
                             wxASCII_STR(wxMessageBoxCaptionStr),
                             wxOK | wxCENTRE | icon_style};
  if (!details.empty()) {
    dialog.ShowDetailedText(details);
  }
  dialog.ShowModal();

  if (type == engine::Presenter::MessageType::kFatalError) {
    main_window_->Close();
  }
}

void WxPresenter::ShowMainWindow() {
  main_window_->Show();
}

}  // namespace ui::wx
