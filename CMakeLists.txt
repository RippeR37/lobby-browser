set(VCPKG_TARGET_TRIPLET x64-windows-static-md)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
  endif()
endif()


###
### Lobby Browser
###

cmake_minimum_required(VERSION 3.13)
project(lobby_browser VERSION 1.0 LANGUAGES CXX)


###
### Dependencies
###

find_package(libbase CONFIG REQUIRED net wx)
find_package(wxWidgets CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Steamworks
set(STEAMWORKS_SDK_DIR "third_party/steam")
include_directories("${STEAMWORKS_SDK_DIR}/include")
link_directories("${STEAMWORKS_SDK_DIR}/lib")

# Verify that Steamworks SDK is provided
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${STEAMWORKS_SDK_DIR}/bin/steam_api64.dll")
  message(FATAL_ERROR "Steamworks SDK not found: missing `steam_api64.dll`")
endif()
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${STEAMWORKS_SDK_DIR}/lib/steam_api64.lib")
  message(FATAL_ERROR "Steamworks SDK not found: missing `steam_api64.lib`")
endif()
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${STEAMWORKS_SDK_DIR}/include/steam_api.h")
  message(FATAL_ERROR "Steamworks SDK not found: missing `steam_api.h`")
endif()


###
### Main target
###

# lobby_browser
add_executable(lobby_browser
    WIN32
    "")

target_compile_features(lobby_browser
  PUBLIC
    cxx_std_17
)

target_compile_definitions(lobby_browser
  PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:_CRT_SECURE_NO_WARNINGS>
)

set_target_properties(lobby_browser
  PROPERTIES
    CXX_EXTENSIONS OFF
)

target_include_directories(lobby_browser
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/>
)

target_compile_options(lobby_browser
  PRIVATE
    /W4
    /WX
    /permissive-
)

target_link_libraries(lobby_browser
  PRIVATE
    libbase::libbase
    libbase::libbase_net
    libbase::libbase_wx
    wx::core
    wx::base
    wx::net
    wx::webview
    wx::propgrid
    nlohmann_json::nlohmann_json
    steam_api64
)

target_link_directories(lobby_browser
  PUBLIC
    ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows-static-md/$<$<CONFIG:Debug>:debug>/lib/
)

target_sources(lobby_browser
  PRIVATE
    src/application.cc
    src/application.h
    src/engine/backends/auth_backend.h
    src/engine/backends/eos/eos_data_serialize.h
    src/engine/backends/eos/eos_data_serialize.cc
    src/engine/backends/eos/eos_data.h
    src/engine/backends/eos/eos_lobby_backend.cc
    src/engine/backends/eos/eos_lobby_backend.h
    src/engine/backends/modio/modio_data.h
    src/engine/backends/modio/modio_data_serialize.cc
    src/engine/backends/modio/modio_data_serialize.h
    src/engine/backends/modio/modio_mod_backend.cc
    src/engine/backends/modio/modio_mod_backend.h
    src/engine/backends/lobby_backend.h
    src/engine/backends/nakama/nakama_data.h
    src/engine/backends/nakama/nakama_data_serialize.cc
    src/engine/backends/nakama/nakama_data_serialize.h
    src/engine/backends/nakama/nakama_lobby_backend.cc
    src/engine/backends/nakama/nakama_lobby_backend.h
    src/engine/backends/steam/steam_auth_backend.cc
    src/engine/backends/steam/steam_auth_backend.h
    src/engine/backends/steam/steam_in_process_auth_backend.cc
    src/engine/backends/steam/steam_in_process_auth_backend.h
    src/engine/backends/steam/steam_manager.cc
    src/engine/backends/steam/steam_manager.h
    src/engine/backends/steam/steam_subprocess_auth_backend.cc
    src/engine/backends/steam/steam_subprocess_auth_backend.h
    src/engine/config/config_loader.cc
    src/engine/config/config_loader.h
    src/engine/engine.h
    src/engine/engine_impl.cc
    src/engine/engine_impl.h
    src/engine/games/base_game.cc
    src/engine/games/base_game.h
    src/engine/games/game.h
    src/engine/games/contractors/contractors_data.cc
    src/engine/games/contractors/contractors_data.h
    src/engine/games/contractors/contractors_game.cc
    src/engine/games/contractors/contractors_game.h
    src/engine/games/pavlov/pavlov_data.cc
    src/engine/games/pavlov/pavlov_data.h
    src/engine/games/pavlov/pavlov_game.cc
    src/engine/games/pavlov/pavlov_game.h
    src/engine/presenter.h
    src/main.cc
    src/models/auth.h
    src/models/config.h
    src/models/game.h
    src/models/search.h
    src/models/serialize/config.cc
    src/models/serialize/config.h
    src/models/serialize/auth.cc
    src/models/serialize/auth.h
    src/ui/event_handler.h
    src/ui/presenter.cc
    src/ui/presenter.h
    src/ui/wx/wx_auto_search_dialog.cc
    src/ui/wx/wx_auto_search_dialog.h
    src/ui/wx/wx_game_page.cc
    src/ui/wx/wx_game_page.h
    src/ui/wx/wx_main_window.cc
    src/ui/wx/wx_main_window.h
    src/ui/wx/wx_presenter.cc
    src/ui/wx/wx_presenter.h
    src/ui/wx/wx_results_list_model.cc
    src/ui/wx/wx_results_list_model.h
    src/ui/wx/wx_tray_icon.cc
    src/ui/wx/wx_tray_icon.h
    src/ui/wx/wx_ui_config.cc
    src/ui/wx/wx_ui_config.h
    src/utils/arg_parse.cc
    src/utils/arg_parse.h
    src/utils/strings.cc
    src/utils/strings.h
    $<$<BOOL:${WIN32}>:src/utils/subprocess_win.cc>
    src/utils/subprocess.h
    src/utils/timer.cc
    src/utils/timer.h
    src/utils/vectors.h

    resources/resources.rc
)


add_executable(steam_auth_client "")

target_compile_features(steam_auth_client
  PUBLIC
    cxx_std_17
)

target_compile_definitions(steam_auth_client
  PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:_CRT_SECURE_NO_WARNINGS>
)

set_target_properties(steam_auth_client
  PROPERTIES
    CXX_EXTENSIONS OFF
)

target_include_directories(steam_auth_client
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/>
)

target_compile_options(steam_auth_client
  PRIVATE
    /W4
    /WX
    /permissive-
)

target_link_libraries(steam_auth_client
  PRIVATE
    libbase::libbase
    nlohmann_json::nlohmann_json
    steam_api64
)

target_sources(steam_auth_client
  PRIVATE
    src/engine/backends/auth_backend.h
    src/engine/backends/steam/steam_auth_backend.cc
    src/engine/backends/steam/steam_auth_backend.h
    src/engine/backends/steam/steam_in_process_auth_backend.cc
    src/engine/backends/steam/steam_in_process_auth_backend.h
    src/engine/backends/steam/steam_manager.cc
    src/engine/backends/steam/steam_manager.h
    src/engine/backends/steam/steam_subprocess_auth_backend.cc
    src/engine/backends/steam/steam_subprocess_auth_backend.h
    src/engine/backends/steam/steam_subprocess_main.cc
    src/models/auth.h
    src/models/serialize/auth.cc
    src/models/serialize/auth.h
    src/utils/arg_parse.cc
    src/utils/arg_parse.h
    src/utils/subprocess.h
    $<$<BOOL:${WIN32}>:src/utils/subprocess_win.cc>
)

add_dependencies(lobby_browser steam_auth_client)

# Copy binaries to output dir
if (WIN32)
    add_custom_command(TARGET lobby_browser POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/${STEAMWORKS_SDK_DIR}/bin/steam_api64.dll"
        "$<TARGET_FILE_DIR:lobby_browser>")
endif()
