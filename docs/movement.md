# Movement

Movement features combine method hooks, runtime metadata, and reversible state patches. They operate only on `PlayerController.LocalInstance` and leave remote players untouched.

## Camera-relative high-speed movement

High-speed movement runs after the local player's normal `Update` method. It reads the current WASD state, obtains the active camera forward direction, removes its vertical component, and derives a horizontal right vector. The combined input vector is normalized so diagonal input is not faster than straight movement.

Configured speed is multiplied by Unity frame delta time before being added to the local transform position. That scaling keeps the requested travel rate stable across different frame rates. If input, camera state, or transform lookup is unavailable, the feature does nothing for that frame.

## Auto sprint

Auto sprint sets the local `PlayerController` sprint-state field immediately before and after its original `Update`. The raw field location is centralized in `game_offsets.h`; remote players and operating-system input are not modified.

## Gravity and movement statistics

Gravity control coordinates three layers that the game may consult:

- Unity's global physics gravity vector.
- The local rigidbody's gravity flag and vertical velocity.
- Movement-related entries in the player's runtime statistics dictionary.

No-gravity mode clears the relevant effects. Custom-gravity mode applies the bounded configured value and uses frame delta time when adjusting vertical velocity. Other controller statistics are patched only when their semantic key identifies the intended movement or gravity property.

## Restoration

Before the first persistent write, Hackmatch records the original global gravity, rigidbody state, and controller statistic values it touches. Disabling the relevant option restores those values. Losing the local player also triggers restoration so state does not leak across lobby or respawn transitions.

Unload performs an explicit final restoration before render resources and the DLL are released. Item values, movement state, statistic patches, and global gravity are returned to their saved values even if the menu options were still enabled.

Raw field locations used by the statistics path are maintained only in [`game_offsets.h`](../src/core/game_offsets.h). The external update process is described in [updating bindings](updating-bindings.md).
