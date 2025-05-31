## Lobby Browser

**Lobby Browser** is a simple accessibility application allowing players to
quickly browse online lobbies (and/or servers) for their games without having to
launch then. This is particularly useful for games with small player base where
its not always possible to find a game or ones that require extra setup (e.g.
VR games).

![Lobby Browser - Pavlov lobbies and servers](https://i.imgur.com/X3zSY72.png)

### End-user Features

* Supported games
  * [Pavlov](https://www.vankrupt.com/#pavlov-vr) (servers, lobbies, requires
    owning the game on Steam)
  * [Contractors](https://www.contractorsvr.com/home-1) (lobbies, requires
    owning the game on Steam)

* Quality of life features
  * Auto-search - looped search with specific criterias until there are matching
    results
    * You can specify minimum number of players or `|`-separated substrings to
      match game modes, map names, etc

### Project Features

* Modular design
  * Easily exchangeable UI
  * Easily extendable for other lobby backend systems or games
* Steam authentication in subprocess
* Multi-game support

## How to use

1. Download the latest version from the
   **[Releases](https://github.com/RippeR37/lobby-browser/releases)** page.
2. Find a `steam_api64.dll` from one the games you own and copy it next to the
   `lobby_browser.exe`.
    * This file is not provided for you here due to unknown licensing conditions.
    * You can find it e.g. in:
        * `...\Steam\steamapps\common\PavlovVR\Pavlov\Binaries\Win64\steam_api64.dll`
3. Run `lobby_browser.exe`

> [!IMPORTANT]
> Steam games requires Steam running in the background and you owning the game
> for most of the functionality to work.

## Build

Project is written in C++ and requires:

* C++ compiler with C++17 support,
  * Tested with MSVC 2022 and Clang 19/20
* CMake
* vcpkg
* Steamworks SDK (you need to provide it and place in `third_party\steam\`)

## Contribute

If you wish to contribute, please open an Issue first and propose changes there.
Be sure to match code style and design of existing modules in the project.
