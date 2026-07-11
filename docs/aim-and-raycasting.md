# Aim and raycasting

Hackmatch redirects only shot-related physics queries made by the local player. The aim path is deliberately narrower than a global raycast override so unrelated camera, movement, and world queries retain the game's original behavior.

## Local-shot detection

The two player shot methods are hooked because the game has separate primary and secondary firing paths. Each detour compares the firing player with `PlayerController.LocalInstance`. When aim is active and the shot belongs to that instance, a thread-local depth counter marks nested physics work as eligible for redirection. A short monotonic-time handoff also covers physics calls that immediately follow the shot method on another call boundary.

Remote-player shots never enter this state. The original shot method always runs, and the depth counter is unwound after it returns.

## Target scoring and FOV filtering

Candidates come from live player controllers and exclude the local player. Team and validity checks happen before scoring. For each candidate, Hackmatch normalizes the direction from the shot origin to a candidate hit point and computes its dot product with the current camera or shot direction.

The largest dot product is the target closest to the view axis. The configured field of view is converted into a minimum dot product, allowing the hot path to compare a scalar rather than project every candidate to screen space. “Ignore field of view” removes that lower bound but retains the same best-angle scoring.

## Hit scanning and wall counting

Shot selection checks several useful points on a player rather than relying on a single transform origin. Each candidate point is scanned with the game's `Physics.RaycastAll` metadata binding. Hits are classified as the target player, the local player, or intervening geometry.

Intervening surfaces are deduplicated by hit distance before they are counted; multiple colliders at nearly the same depth should represent one barrier, not unfairly exhaust the wall allowance. A point is accepted only when the target is found and the number of distinct surfaces before it is within the active setting's allowance.

## Shot validation

After a target wins scoring, Hackmatch builds a normalized direction from the real ray origin to the selected hit point. Before substituting it, the code calls the original single-hit raycast directly to confirm the redirected path produces a valid physics hit. Calling the saved original function also prevents the validation query from recursively entering the detour.

If target lookup, scanning, normalization, or validation fails, the original direction passes through unchanged.

## Hooked raycast variants

The game can select any of three Unity physics entry points during firing, so all three receive the same guarded redirection policy:

- `Physics.Raycast` returns whether the query hit anything.
- `Physics.RaycastAll` returns every hit along the query.
- `Physics.RaycastNonAlloc` fills a caller-owned results array.

Only the direction argument changes. Origins, distances, layer masks, trigger policy, result storage, return values, and original call order are preserved.

The version-sensitive method locations used to install these hooks belong only in [`game_offsets.h`](../src/core/game_offsets.h). See [updating bindings](updating-bindings.md) before changing them.
