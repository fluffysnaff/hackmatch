# Agent guide

Hackmatch is a Windows-only C++20 DLL for controlled Redmatch 2 research. It hooks IL2CPP and DirectX 11, renders an ImGui menu, and restores persistent game changes on disable or unload. Keep changes small, preserve the current architecture, and never commit game binaries or generated metadata.

## Start here

1. Read `README.md` for supported features and user-facing commands.
2. Read `CONTRIBUTING.md` for build, test, preview, and binding-update requirements.
3. Read the relevant guide in `docs/` before changing movement, aim, raycasts, or raw bindings.
4. For `PlayerController` behavior or binding work, use `reverse_docs/README.md` when the ignored local research tree exists; otherwise start at `docs/game-re/README.md`. Use the linked contract rather than repeating static analysis or selecting a hook from an RVA alone.
5. Check `git status` before editing. The worktree may contain concurrent user changes; do not revert them.

## Repository map

- `src/core`: settings, profiles, IL2CPP exports, game classes, logging, and all version-sensitive constants in `game_offsets.h`.
- `src/features`: gameplay patches, item restoration, ESP, targeting, and pure feature limits.
- `src/platform`: MinHook lifecycle, DirectX swap-chain hooks, rendering setup, and unload handling.
- `src/ui`: ImGui menu, widgets, themes, SVG logo resource, and deterministic preview executable.
- `tests`: small assert-based tests for portable logic and configuration compatibility.
- `docs`: behavior guides plus the tracked, source-used binding evidence in `docs/game-re`.
- `reverse_docs`: optional ignored workspace containing the full local reversing catalog; prefer it over the compact tracked fallback when present.
- `scripts`: release build, game discovery, installation, and installer fixtures.

## Invariants

- Resolve stable IL2CPP members by semantic metadata name. Put unavoidable RVAs and raw field offsets only in `src/core/game_offsets.h`, document their evidence in `docs/game-re`, and update `supported_build` with verified values.
- A function contract must explicitly permit the intended call or preserved hook; an RVA and understood behavior alone are not integration permission.
- Treat raw IL2CPP containers, strings, boxed values, `RaycastHit`, and `MethodInfo` layouts as version-sensitive bindings.
- Treat hook RVAs, virtual addresses, and file offsets as different values. Follow `docs/updating-bindings.md` and use matching `GameAssembly.dll` and `global-metadata.dat` files.
- Restrict gameplay changes to `PlayerController.LocalInstance` unless a feature explicitly targets remote players.
- Snapshot every persistent value before changing it. Restore on feature disable, player/session loss, and `Gameplay::restore()` during unload.
- Keep render hooks responsible for lifecycle only. Keep game behavior in `src/features` and menu behavior in `src/ui`.
- Theme enum order, `theme_keys`, and `built_in_themes` order must match. Add new themes immediately before `Custom` so existing named profile values remain readable.
- `images/logo.svg` is the menu logo source. It is embedded by `src/ui/assets.rc` and rasterized once into a DX11 texture; do not replace it with procedural ImGui drawing.

## Verification

Run the narrowest useful checks, then the release checks before finishing:

```bat
scripts\build_release.bat
ctest --test-dir build-release -C Release --output-on-failure
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\test_installer.ps1
```

For UI changes, also build `hackmatch_ui_preview` and render affected pages and themes using the command documented in `CONTRIBUTING.md`. A successful native preview verifies the production ImGui path without injecting into the game. Runtime hook or binding changes still require an explicit in-game check against the build recorded in `game_offsets.h`; state what was not tested.

## Change discipline

- Build with warnings treated as errors and follow `.clang-format`.
- Prefer existing helpers and platform APIs over new dependencies or abstractions.
- Add one focused regression check for non-trivial portable logic. Do not create fake tests for game-memory behavior that only runtime injection can prove.
- Update user docs when controls, features, theme counts, assets, build steps, or supported bindings change.
- Never commit `build*`, `out`, dumps, profiles, DLLs, PDBs, Steam files, or proprietary game data.
