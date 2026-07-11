# Hackmatch: Redmatch 2 Cheat

[![Platform: Windows](https://img.shields.io/badge/platform-Windows-0078D4)](#requirements)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C)](CMakeLists.txt)
[![GitHub release](https://img.shields.io/github/v/release/fluffysnaff/hackmatch?display_name=tag&sort=semver)](https://github.com/fluffysnaff/hackmatch/releases/latest)
[![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0-D22128)](LICENSE)

<p align="center"><img src="images/01_hackmatch_menu_aim.png" alt="Hackmatch Aim menu"></p>

## 🚀 Get Started

### ➡️ [Download the Latest Release](https://github.com/fluffysnaff/hackmatch/releases/latest)

Download and extract the latest release, close Redmatch 2, then run:

```bat
install.bat
```

> **Quick tip:** After successfully installing Hackmatch, start Redmatch 2 and press `Insert` to open or close the menu.

See the [installation guide](docs/installation.md) for custom game paths, manual installation, what the script does, and solutions to common errors.

## 🔎 Overview

Hackmatch is an open-source, Windows-only **Redmatch2 hack** written in C++20. Also known as an RM2 cheat, it adds aim assistance, a player ESP overlay, weapon and movement options, configurable hotkeys, profiles, and themes through an in-game DirectX 11 ImGui interface.

See the [complete menu gallery](images/README.md) for every module.

## 📑 Contents

- [Redmatch 2 mod features](#features)
- [Technology](#technology)
- [Source build requirements](#requirements)
- [Build from source](#building)
- [Hotkeys and controls](#controls)
- [Documentation](#documentation)
- [FAQ](#faq)
- [Project](#project)

<a id="features"></a>

## 🎮 Redmatch 2 Mod Features

| Module | Current options |
| --- | --- |
| 🎯 Aim | Configurable activation, FOV filtering, target selection, wall checks, and shot redirection |
| 👁️ Visuals | Full/corner boxes, names, distance, snaplines, offscreen arrows, target markers, and inherited/custom colors |
| 🔫 Weapons | No spread, infinite ammo, instant reload, no camera shake, rapid fire, and custom damage |
| 🏃 Player | Auto sprint, camera-relative speed, gravity controls, camera FOV, and spawn-protection visibility |
| ⚙️ Config | Named profiles, copy-and-paste config sharing, configurable hotkeys, menu opacity, 36 built-in palettes, and a custom theme editor |

Built-in coding palettes include Dracula, Catppuccin, Tokyo Night, Nord, Gruvbox, One Dark, Solarized, Monokai, GitHub Dark, Rose Pine, Kanagawa, Everforest, Synthwave '84, Night Owl, Poimandres, Vesper, Andromeeda, Aurora X, Ayu Dark, Dark Plus, Houston, Laserwave, Material, Slack Dark, and Vitesse Dark variants. Choose **Custom** in Config to edit its background, surface, accent, text, and muted colors. Use **Copy current config** and **Paste config** to share settings. Themes and settings are saved with profiles in `%APPDATA%\Hackmatch\profiles`.

<a id="technology"></a>

## 🧩 Technology

Hackmatch is a native C++20 DLL that works with Redmatch 2's IL2CPP runtime. It uses MinHook for hook lifecycle, DirectX 11 for rendering, Dear ImGui for the menu, and CMake with Ninja for reproducible builds.

<a id="requirements"></a>

## ✅ Source Build Requirements

- Windows 10 or Windows 11
- Redmatch 2 on Steam
- Visual Studio 2022 C++ Build Tools
- CMake 3.24 or newer
- Ninja

<a id="building"></a>

## 🔨 Build from Source

Install the requirements above, then clone and build the repository:

```bat
git clone https://github.com/fluffysnaff/hackmatch.git
cd hackmatch
scripts\build_release.bat
```

The release DLL is written to `build-release\hackmatch.dll`. To build and install in one step, optionally providing a custom game folder:

```bat
build_and_install.bat
build_and_install.bat "C:\Program Files (x86)\Steam\steamapps\common\Redmatch 2"
```

Preview installation without changing files with `scripts\install_enforcer.bat --dry-run`. See the [installation guide](docs/installation.md) for full instructions and troubleshooting.

At startup, Hackmatch compares Steam's installed build ID with the build recorded in `game_offsets.h`. A mismatch is reported in the menu and console but does not prevent startup. Each launch rewrites `%APPDATA%\Hackmatch\hackmatch.log` with the current console session.

<a id="controls"></a>

## ⌨️ Hotkeys and Controls

| Key | Action |
| --- | --- |
| `Insert` | Toggle the menu (configurable) |
| `End` | Restore modified state, remove hooks, and unload (configurable) |

Aim and ESP can also be assigned toggle hotkeys from Config. Hotkeys accept keyboard or mouse buttons; `Esc`, Windows, menu, and sleep keys are reserved.

<a id="documentation"></a>

## 📚 Documentation

- [Aim and raycasting](docs/aim-and-raycasting.md)
- [Installation and troubleshooting](docs/installation.md)
- [Movement](docs/movement.md)
- [Scripts](docs/scripts.md)
- [Updating game bindings](docs/updating-bindings.md)
- [Contributing and UI previews](CONTRIBUTING.md)

<a id="faq"></a>

## ❓ FAQ

### What is Hackmatch?

Hackmatch is a native Redmatch 2 hack for Windows. It provides aim, ESP, weapon, player, appearance, profile, and hotkey settings in an in-game menu.

### How do I install Hackmatch?

Download and extract the latest release, then run `install.bat`. It finds the Redmatch 2 Steam installation and installs the packaged DLL as `Redmatch 2_Data\enforcer.dll`. Source builds can use `build_and_install.bat` instead.

### Does Hackmatch support every Redmatch 2 version?

IL2CPP bindings can change when Redmatch 2 updates. Hackmatch compares the installed Steam build with its supported build and displays a warning when they do not match. See [Updating game bindings](docs/updating-bindings.md) for the verification process.

### Are gameplay changes permanent?

Gameplay changes apply only while enabled. Hackmatch restores the original values when a feature is disabled or the DLL is unloaded.

### Where are Hackmatch profiles stored?

Profiles, themes, hotkeys, and other settings are stored in `%APPDATA%\Hackmatch\profiles`. Configurations can also be shared using **Copy current config** and **Paste config** in the Config module.

<a id="project"></a>

## ⚖️ Project

Hackmatch is an independent educational project, not affiliated with Redmatch 2. Use it only where you have permission and understand the game's rules. Licensed under [GPL-3.0](LICENSE); third-party notices are in [NOTICE](NOTICE).

## Star History

<a href="https://www.star-history.com/?type=date&repos=fluffysnaff%2Fhackmatch">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/chart?repos=fluffysnaff/hackmatch&type=date&theme=dark&legend=top-left&sealed_token=6XwskM4RBkrUZ2d0yKXhYPHa2WuCwIZFuzy0bCD4UQnOY_MmxxwmwdLFrt6N4pqcTISHbz5W5Vw-Vt61Q1mR4An_7pGGs4CG6jlQLVdWe28hwefH0ljHEUsicJLnE3X0AZ6bfzt8lIlJTpIbLJbxU_XaC-K0AbApQX5qR_ma81QjSNaf7SOIdgnnDfpf" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/chart?repos=fluffysnaff/hackmatch&type=date&legend=top-left&sealed_token=6XwskM4RBkrUZ2d0yKXhYPHa2WuCwIZFuzy0bCD4UQnOY_MmxxwmwdLFrt6N4pqcTISHbz5W5Vw-Vt61Q1mR4An_7pGGs4CG6jlQLVdWe28hwefH0ljHEUsicJLnE3X0AZ6bfzt8lIlJTpIbLJbxU_XaC-K0AbApQX5qR_ma81QjSNaf7SOIdgnnDfpf" />
   <img alt="Star History Chart" src="https://api.star-history.com/chart?repos=fluffysnaff/hackmatch&type=date&legend=top-left&sealed_token=6XwskM4RBkrUZ2d0yKXhYPHa2WuCwIZFuzy0bCD4UQnOY_MmxxwmwdLFrt6N4pqcTISHbz5W5Vw-Vt61Q1mR4An_7pGGs4CG6jlQLVdWe28hwefH0ljHEUsicJLnE3X0AZ6bfzt8lIlJTpIbLJbxU_XaC-K0AbApQX5qR_ma81QjSNaf7SOIdgnnDfpf" />
 </picture>
</a>
