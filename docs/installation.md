# Installation Guide

This guide explains the automatic installer, manual installation, and common build or installation errors.

## Automatic Installation

### Requirements

- Windows 10 or Windows 11
- Redmatch 2 installed through Steam

Download and extract the [latest release](https://github.com/fluffysnaff/hackmatch/releases/latest), close Redmatch 2, and run:

```bat
install.bat
```

If the game is not found automatically, provide the folder containing `Redmatch 2.exe`:

```bat
install.bat "C:\Program Files (x86)\Steam\steamapps\common\Redmatch 2"
```

Start Redmatch 2 and press `Insert` to open the menu.

## What the Script Does

`install.bat` performs the following steps:

1. Starts `scripts\install_enforcer.bat`.
2. Validates the packaged `hackmatch.dll`.
3. Finds Redmatch 2 through the supplied path or registered Steam libraries.
4. Validates that the folder contains `Redmatch 2.exe` and `Redmatch 2_Data`.
5. Copies the DLL through a temporary file and replaces `Redmatch 2_Data\enforcer.dll`.

The installer does not delete unrelated game files. Use `scripts\install_enforcer.bat --dry-run` to preview its source and destination without changing files.

## Manual Installation

These commands reproduce the automatic process:

```bat
copy /Y "hackmatch.dll" "C:\path\to\Redmatch 2\Redmatch 2_Data\enforcer.dll"
```

Replace the example path with the real Redmatch 2 folder. The destination filename must be `enforcer.dll`. Close the game before replacing an existing DLL.

You can also run the underlying installer directly:

```bat
scripts\install_enforcer.bat "C:\path\to\Redmatch 2"
```

## Building from Source

Source builds additionally require Visual Studio 2022 C++ Build Tools with the **Desktop development with C++** workload, CMake 3.24 or newer, and Ninja on `PATH`.

```bat
git clone https://github.com/fluffysnaff/hackmatch.git
cd hackmatch
build_and_install.bat
```

`build_and_install.bat` runs `scripts\build_release.bat` first, producing `build-release\hackmatch.dll`, then runs the same installer used by the release package.

## Common Errors

### `cmake` is not recognized

Install CMake 3.24 or newer, enable its option to add CMake to `PATH`, then open a new Command Prompt.

### Ninja cannot be found

Install Ninja and add its folder to `PATH`. Confirm it works with `ninja --version`.

### No C++ compiler is available

Install Visual Studio 2022 Build Tools with the **Desktop development with C++** workload. The build script checks the default Build Tools installation path; use a Visual Studio Developer Command Prompt if Visual Studio is installed elsewhere.

### Redmatch 2 was not found

Pass the game folder explicitly. Use the folder containing `Redmatch 2.exe`, not the executable itself:

```bat
install.bat "D:\SteamLibrary\steamapps\common\Redmatch 2"
```

### Missing or invalid `hackmatch.dll`

The release was not fully extracted or the DLL is damaged. Extract a fresh download before installing. For a source build, run `scripts\build_release.bat` and resolve the first reported build error.

### The DLL cannot be copied or replaced

Close Redmatch 2 so `enforcer.dll` is not locked. If Windows still denies access, open Command Prompt as administrator and try again.

### Unsupported game build warning

Installation succeeded, but the installed Redmatch 2 build differs from the version recorded in `src\core\game_offsets.h`. Version-sensitive features may require updated bindings; see [Updating game bindings](updating-bindings.md).
