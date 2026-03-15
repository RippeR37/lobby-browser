#include "ui/wx/wx_web_view_members_template.h"

#include "utils/strings.h"

namespace ui::wx {

std::string WxWebViewMembersHeader(UiEffectiveTheme theme,
                                   const std::string& game_name) {
  (void)game_name;

  std::string result = R"(
<!DOCTYPE html>
<html lang='en'>

<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <style>
    body:hover {
      cursor: default;

      -webkit-user-select: none; /* Safari */
      -ms-user-select: none; /* IE 10 and IE 11 */
      user-select: none; /* Standard syntax */
    }

    $EXTRASTYLES
  </style>
</head>

<body style='margin: 0px;'>
  <table width='100%'>

)";

  switch (theme) {
    case UiEffectiveTheme::Light: {
      util::ReplaceAll(result, "$EXTRASTYLES", "");
    } break;

    case UiEffectiveTheme::Dark: {
      const std::string dark_mode_styles = R"(
    body {
      color: #cccccc;
      background-color: #2a2a2a;
    }
)";
      util::ReplaceAll(result, "$EXTRASTYLES", dark_mode_styles);
    } break;
  }

  return result;
}

std::string WxWebViewMembersMemberRow(
    UiEffectiveTheme theme,
    const model::SearchDetailsResponse::Member& member,
    const std::string& game_name) {
  if (game_name == "Quake 3") {
    std::string score = "0";
    std::string ping = "0";
    bool is_bot = false;

    for (const auto& data : member.user_data) {
      if (data.first == "Score")
        score = data.second;
      else if (data.first == "Ping")
        ping = data.second;
      else if (data.first == "IsBot")
        is_bot = (data.second == "true");
    }

    std::string result = R"(
    <tr style='border-bottom: 1px solid #444;'>
      <td style='padding-left:10px; padding-top: 5px;' valign='top'>
        <span style='font-size:16px; font-weight:bold; color: $NAMECOLOR;'>$USERNAME</span><br>
        <span style='font-size:11px; color: #888;'>Ping: $PING ms</span>
      </td>
      <td align='right' valign='middle' style='padding-right: 10px;'>
        <span style='font-size:20px; font-weight:bold; font-family: monospace; color: #f90;'>$SCORE</span>
      </td>
    </tr>
)";

    util::ReplaceAll(result, "$USERNAME", member.name);
    util::ReplaceAll(result, "$SCORE", score);
    util::ReplaceAll(result, "$PING", ping);
    switch (theme) {
      case UiEffectiveTheme::Light:
        util::ReplaceAll(result, "$NAMECOLOR", is_bot ? "#666" : "#000");
        break;
      case UiEffectiveTheme::Dark:
        util::ReplaceAll(result, "$NAMECOLOR", is_bot ? "#aaa" : "#fff");
        break;
    }

    return result;
  }

  // Default (Pavlov) template
  std::string result = R"(
    <tr>
      <td width='48'>
        <img width='48' height='48' src='$USERAVATAR' alt='' title='$USERNAME'>
      </td>
      <td style='padding-left:10px;'>
        <img width='14' height='14' src='$USERICON' onclick='window.wxWebView.postMessage("defaultBrowserNavigate;$USERPROFILE")'>
        <span style='font-size:18px;font-weight:bold;'>$USERNAME</span><br>
        <span style='font-size:10px;font-family:monospace;' onclick='window.wxWebView.postMessage("uiNavigate;player;$USERID")'>$USERPLATFORMID</span>
      </td>
    </tr>
)";

  util::ReplaceAll(result, "$USERNAME", member.name);
  util::ReplaceAll(result, "$USERID", member.id);
  util::ReplaceAll(result, "$USERPLATFORMID", member.platform_id);
  util::ReplaceAll(result, "$USERAVATAR", member.avatar_url);
  util::ReplaceAll(result, "$USERICON", member.icon_url);
  util::ReplaceAll(result, "$USERPROFILE", member.profile_url);

  return result;
}

std::string WxWebViewMembersFooter() {
  return R"(

  </table>
</body>

</html>

)";
}

}  // namespace ui::wx
