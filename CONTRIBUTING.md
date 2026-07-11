# Contributing to Hackmatch

Thanks for helping improve Hackmatch. Small, focused changes are easier to review and much less likely to hide a version-sensitive regression.

## Before opening a pull request

1. Build with Visual Studio 2022 Build Tools, CMake, and Ninja:

   ```bat
   scripts\build_release.bat
   ```

2. Run the C++ and installer tests:

   ```bat
   ctest --test-dir build-release -C Release --output-on-failure
   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\test_installer.ps1
   ```

3. Keep code formatted according to `.clang-format` and ensure project code compiles with warnings treated as errors.

## UI visual verification

Build the native preview target to review the real ImGui widgets without launching or modifying the game:

```bat
cmake --build build-release --target hackmatch_ui_preview
build-release\hackmatch_ui_preview.exe out\hackmatch-ui.png 0 0 952 692 1 1 0
```

Arguments after the output select theme (`0`–`36`, custom last), module (`0`–`4`), width, height, navigation progress, content progress, and reduced motion. Inspect every affected page at its rendered size; the preview uses the production theme, fonts, settings, and widgets.

## Code guidelines

- Prefer clear types and names over comments that restate code.
- Keep settings in the appropriate grouped settings structure.
- Resolve stable IL2CPP metadata by name; keep unavoidable raw values in `game_offsets.h`.
- Restore every persistent state change when its feature is disabled or Hackmatch unloads.
- Do not commit build output, game binaries, generated dumps, or local Visual Studio files.
- Avoid unrelated formatting or refactors in a behavior-changing pull request.

## Binding updates

Read [docs/updating-bindings.md](docs/updating-bindings.md). Include the Redmatch 2 build identifier, old and new values, generated signatures and types used as evidence, and the runtime checks performed. Keep current numeric values in `src/core/game_offsets.h` rather than duplicating them in documentation.

## Reports

Bug reports should include the game build identifier, Hackmatch commit, build configuration, reproduction steps, expected behavior, actual behavior, and relevant console output. Do not attach proprietary game binaries or complete generated metadata output.
