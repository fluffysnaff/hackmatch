# Updating bindings

Game updates can move native methods, change managed layouts, and replace useful metadata names. Treat a binding refresh as one evidence-backed update: identify every required category, change the centralized constants together, then validate the complete runtime lifecycle.

## Generate current metadata externally

Copy the installed `GameAssembly.dll` and the matching `global-metadata.dat` into a separate working directory. Run [Il2CppDumper](https://github.com/Perfare/Il2CppDumper) against that pair. Its generated C# declarations, native header, and address maps are reference material for the update; they are not project inputs and should not be committed.

Record a durable identifier for the exact game release you inspected. The identifier and all current numeric bindings remain authoritative only in [`game_offsets.h`](../src/core/game_offsets.h).

## Required binding categories

Review every category even if the affected feature appears to work:

- Both local-player shot methods, including their shot-info parameter type.
- The single-hit, all-hits, and non-allocating Unity raycast overloads, including complete signatures.
- Player identity, display-name, and team relationships used by the overlay.
- The player statistics container and sprint-state fields used by movement.
- Stable classes, lifecycle methods, item data, camera, shield, and local-instance fields resolved through metadata names.

Prefer runtime lookup by stable semantic name. Add or retain a raw constant only when metadata lookup cannot identify a required binding reliably.

## Recover semantic names

Obfuscation may replace a meaningful field or method name while leaving its role recognizable. Compare the new metadata with a known-good release using:

- Declared type and generic arguments.
- Field order, size, and neighboring stable members.
- Method parameter and return types.
- Call sites and native cross-references.
- Observable runtime behavior under a debugger.

Keep recovered names in Hackmatch wrappers and documentation, not by editing generated output. A plausible position in a class is supporting evidence, not proof; corroborate it with type and usage before enabling the feature.

## RVA versus file offset

Hook constants are relative virtual addresses: the runtime adds them to the loaded `GameAssembly.dll` base. An image-base virtual address must first be converted to an RVA. A raw file offset describes bytes on disk and cannot be substituted because executable sections have different file and memory layouts.

Check the labels emitted by the tool and confirm the module base calculation. For overloaded Unity methods, match the full signature rather than choosing the first method with the right name.

## Validate the update

1. Build Release with warnings treated as errors and run CTest plus the installer fixtures.
2. Inject with every feature disabled and confirm metadata resolution, DirectX readiness, menu toggling, resize handling, and clean unload.
3. Exercise each named metadata path before testing raw-layout features.
4. Validate both shot methods and all three raycast variants with and without FOV and surface filtering.
5. Test item restoration, movement transitions, gravity restoration, respawn/lobby changes, and ESP identity and team data.
6. Confirm the lifecycle log names the exact failing binding or hook when a deliberately invalid value is tested.

If evidence for one binding is incomplete, keep the affected feature disabled and document what remains unknown. Do not guess a value merely because it avoids an immediate crash.
