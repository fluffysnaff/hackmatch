# Aim and raycasting

Hackmatch redirects only shot-related physics queries made by the local player. The aim path is deliberately narrower than a global raycast override so unrelated camera, movement, and world queries retain the game's original behavior.

## Local-shot detection

The verified primary-fire method is hooked and compared with `PlayerController.LocalInstance`. When aim is active and the shot belongs to that instance, a thread-local depth counter marks only its nested physics work as eligible for redirection. The previously labeled secondary-shot RVA is a spread-increment helper and is not hooked.

Remote-player shots never enter this state. The original shot method always runs, and the depth counter is unwound after it returns.

## Target scoring and FOV filtering

Candidates come from live player controllers and exclude the local player. Confirmed teammates are excluded by default using the latest completed ESP snapshot of the game's team-outline metadata; unknown relationships remain targetable for free-for-all and incomplete initialization. For each candidate, Hackmatch normalizes the direction from the shot origin to a candidate hit point and computes its dot product with the current camera or shot direction.

The largest dot product is the target closest to the view axis. The configured field of view is converted into a minimum dot product, allowing the hot path to compare a scalar rather than project every candidate to screen space. “Ignore field of view” removes that lower bound but retains the same best-angle scoring.

The first player selected while aim is active remains locked until the activation key is released. If that player leaves the field of view or becomes obstructed, redirection pauses instead of switching to another player. A locked player with no remaining hittable collider is treated as unavailable and selection retries, allowing a new target after death. The ESP target marker reflects this shot-validated state rather than running a separate angle-only selection.

The optional spawn-protection filter skips players while either the documented paired controller state or `shieldObject` is active. Existing profiles default this filter off.

## Hit scanning and wall counting

Automatic target selection checks head, torso, and base points. A profile can instead restrict selection to one of those points. Each candidate point is scanned with the game's `Physics.RaycastAll` metadata binding. Hits are classified as the target player, the local player, or intervening geometry.

Intervening surfaces are deduplicated by hit distance before they are counted; multiple colliders at nearly the same depth should represent one barrier, not unfairly exhaust the wall allowance. A point is accepted only when the target is found and the number of distinct surfaces before it is within the active setting's allowance.

## Shot validation

After a target wins scoring, Hackmatch builds a normalized direction from the real ray origin to the selected hit point. Before substituting it, the code calls the original single-hit raycast directly to confirm the redirected path produces a valid physics hit. Calling the saved original function also prevents the validation query from recursively entering the detour.

If target lookup, scanning, normalization, or validation fails, the original direction passes through unchanged.

## Hooked raycast variant

The documented primary-fire path uses `Physics.RaycastAll`, so only that variant is hooked. `Physics.Raycast` remains an unhooked direct validation call, and `Physics.RaycastNonAlloc` is left untouched.

- `Physics.RaycastAll` returns every hit along the query.

Only the direction argument changes. Origins, distances, layer masks, trigger policy, result storage, return values, and original call order are preserved.

## FOV guide

The on-screen aim circle uses the current local controller camera FOV after ADS and custom-FOV processing. It is not based on a fixed 90-degree assumption.

The version-sensitive method locations used to install these hooks belong only in [`game_offsets.h`](../src/core/game_offsets.h). See [updating bindings](updating-bindings.md) before changing them.
