# Source-used contracts

## Player lifecycle and ownership

`PlayerController.Start` (`0x1802B30`) establishes the owner/remote split and assigns `PlayerController.LocalInstance` only on the owner path. Features must re-read that static field, tolerate replacement or loss, and never infer authority from an RVA alone.

Allowed: ownership/lifecycle evidence (`OBSERVE_ONLY`). Rejected: manual invocation, forcing the owner branch, or caching the local pointer across sessions.

## Per-frame integration

`PlayerController.Update` (`0x181D2C0`, resolved by metadata) produces local movement `+0x11C`, camera FOV, ADS `+0x161`, sprint `+0x164`, inventory, and weapon state. Remote controllers take an early bookkeeping/spread branch.

Allowed: `HOOK_PRESERVE` with the original call always executed and mutations restricted to the current `LocalInstance`. High-speed movement restores the previous override before the original frame and reapplies it afterward so authentic input gates still run.

## Fixed-step movement

`PlayerController.FixedUpdate` (`0x1801DD0`) writes measured speed `+0x280`, transforms local movement `+0x11C`, computes a Rigidbody destination, raycasts the path, and calls `MovePosition` only when unobstructed.

Allowed: evidence for movement production/consumption and diagnostics (`OBSERVE_ONLY`). Rejected: bypassing its raycast, calling outside fixed timestep, or applying it to remotes.

## Primary fire and raycasts

`FirePrimaryShot` (`0x1814A80`) runs the complete selected-weapon discharge path, including spread, pellet `Physics.RaycastAll` calls, hit aggregation, network/result work, ammo, recoil, and presentation.

Allowed: `HOOK_PRESERVE` solely to establish thread-local scope around the authentic local call. The nested `RaycastAll` hook may replace only direction; origin, range, masks, trigger policy, ordering, results, and original execution remain unchanged. Rejected: fabricated calls, damage/result manipulation, replay, remote players, or client-authority claims.

## Weapon spread

`ComputeWeaponSpread` (`0x1804CD0`) reads Rigidbody speed, selected item/definition, ADS, and accumulated item spread, then writes current spread. It requires a valid body, item array, selected index, item, and definition.

`UpdateCrosshairSpreadLayout` (`0x1808B30`) updates four directional crosshair transforms only while not ADS. It requires all arrays, a valid style index, and non-null elements.

Allowed: `CALL` on the local controller after validating every precondition. Rejected: calling against partially initialized controllers or replacing the native formula.

## Shield presentation/state

The deactivation path (`0x1812F20`) clears `PlayerController +0x10C` and disables `shieldObject`; related activation code sets both. The function also has global presentation dependencies, so Hackmatch mirrors only the paired local writes and does not call it.

Allowed: `BIND` on `LocalInstance` with first-write snapshots and complete restoration. The evidence proves paired status/presentation state, not server-side invulnerability or authority. Rejected: remote writes or claims that the field alone proves protection semantics.

## Sprint state

Update and dedicated set/clear helpers identify `+0x164` as sprint-like state and show normal ADS, reload, input, and definition gates. Auto sprint writes only the local field before and after preserved Update execution.

Allowed: `BIND`, experimental and local-only. Rejected: remote writes or treating the field as network authority.
