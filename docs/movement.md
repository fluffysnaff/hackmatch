# Movement

Movement features combine method hooks, runtime metadata, and reversible state patches. They operate only on `PlayerController.LocalInstance` and leave remote players untouched.

## Collision-preserving high-speed movement

High-speed movement runs after the local player's normal `Update` method. It scales the horizontal component of the game's smoothed local movement intent to the configured speed, preserving the direction, vertical component, input gates, and sprint state produced by the original frame.

The original value is restored before the next `Update`, then the override is regenerated from that frame's authentic input. The unchanged `FixedUpdate` consumes the override through its normal `Rigidbody.MovePosition` and raycast obstruction path, so the feature no longer writes the Transform directly or bypasses the controller's collision gate.

## Camera FOV

Custom FOV runs after the original local `Update` and uses only the camera referenced by `PlayerController.cam`; it does not fall back to `Camera.main`. Before the next frame, Hackmatch restores the game-produced value so ADS and weapon FOV calculations continue normally, then reapplies the configured override after the original frame finishes.

## Diagnostics

The optional Movement Graph HUD plots horizontal Rigidbody velocity and measured fixed-step speed, and labels ADS and sprint state. It reads local controller state after the original `Update`; it does not replace `Update` or `FixedUpdate`, alter raycasts, or assign names to unresolved network values.

## Auto sprint

Auto sprint sets the local `PlayerController` sprint-state field immediately before and after its original `Update`. The raw field location is centralized in `game_offsets.h`; remote players and operating-system input are not modified.

## Spawn protection

Disable own spawn protection mirrors the documented local deactivation effects: it hides `shieldObject` and clears the paired controller state at `+0x10C`. Both values are snapshotted before the first write and restored on disable, local-player replacement, session loss, or unload. The original transition RVA is not called because it also changes global presentation state and has unresolved dependencies.

## Gravity and movement statistics

Gravity control coordinates three layers that the game may consult:

- Unity's global physics gravity vector.
- The local rigidbody's gravity flag and vertical velocity.
- Movement-related entries in the player's runtime statistics dictionary.

No-gravity mode clears the relevant effects. Custom-gravity mode applies the bounded configured value and uses frame delta time when adjusting vertical velocity. Other controller statistics are patched only when their semantic key identifies the intended movement or gravity property.

## Restoration

Before the first persistent write, Hackmatch records the original global gravity, Rigidbody gravity flag, spawn-shield state, and controller statistic values it touches. Disabling the relevant option restores those values. Losing the local player also triggers restoration so state does not leak across lobby or respawn transitions.

Unload performs an explicit final restoration before render resources and the DLL are released. Item values, movement state, statistic patches, and global gravity are returned to their saved values even if the menu options were still enabled.

Raw field locations used by the statistics path are maintained only in [`game_offsets.h`](../src/core/game_offsets.h). The external update process is described in [updating bindings](updating-bindings.md).
