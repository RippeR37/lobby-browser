#include "ui/wx/wx_web_view_members_template.h"

#include "utils/strings.h"

namespace ui::wx {

std::string WxWebViewMembersHeader() {
  return R"(
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
  </style>
</head>

<body style='margin: 0px;'>
  <table width='300'>

)";
}

std::string WxWebViewMembersMemberRow(
    const model::SearchDetailsResponse::Member& member) {
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
