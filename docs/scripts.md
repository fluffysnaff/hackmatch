# Scripts

Hackmatch includes a few Windows scripts for building, finding the game, installing the DLL, and testing the installer.

| Script | Purpose |
| --- | --- |
| `install.bat` | Installs the DLL included in a downloaded release package. |
| `build_and_install.bat` | Builds Hackmatch, then installs it into Redmatch 2. |
| `scripts\build_release.bat` | Configures and builds the release DLL. |
| `scripts\install_enforcer.bat` | Finds Redmatch 2 and installs the built DLL. |
| `scripts\find_redmatch.ps1` | Locates and validates the Redmatch 2 folder. |
| `scripts\test_installer.ps1` | Tests game discovery and installation in a temporary fixture. |

## Build and install

```bat
build_and_install.bat ["C:\path\to\Redmatch 2"]
```

This is the main user-facing script. It runs `scripts\build_release.bat` and stops if the build fails. It then passes any supplied game path to `scripts\install_enforcer.bat`.

## Build release

```bat
scripts\build_release.bat
```

This script:

1. Loads the Visual Studio 2022 Build Tools environment when installed in the default location.
2. Configures CMake with Ninja, release optimizations, and tests enabled.
3. Builds the project into `build-release`.

The resulting DLL is `build-release\hackmatch.dll`.

## Install

```bat
scripts\install_enforcer.bat ["C:\path\to\Redmatch 2"] [--dry-run]
```

The installer uses the supplied path or searches the registered Steam libraries. If discovery fails, it asks for the game folder. A valid folder must contain `Redmatch 2.exe` and `Redmatch 2_Data`.

The built DLL is installed as `Redmatch 2_Data\enforcer.dll`. Close Redmatch 2 before installing so an existing DLL is not locked.

Use `--dry-run` to show the source and destination without changing any files. `--same-window` is intended for other scripts and automated tests so they receive the installer exit code.

## Find the game

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\find_redmatch.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\find_redmatch.ps1 -ExplicitPath "C:\path\to\Redmatch 2"
```

This helper checks explicit paths, the Steam registry entries, `libraryfolders.vdf`, and the Redmatch 2 Steam manifest. On success it prints the validated game folder; on failure it exits with code `1`.

## Test the installer

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\test_installer.ps1
```

Run `scripts\build_release.bat` first. The test creates a temporary fake Steam library, verifies discovery, dry-run and installation behavior, checks locked-file handling, and removes the fixture afterward. It does not modify the real game installation.
