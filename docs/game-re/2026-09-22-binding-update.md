# Binding update: September 22, 2026

Hackmatch 1.2.1 targets Redmatch 2 Steam public build `24496548`, replacing the bindings for `23904900`. This update changes five native method locations and corrects Rigidbody metadata lookups. Existing gameplay, restoration, settings and hook contracts remain in use.

**Validation:** the Release build with warnings treated as errors, all three CTest tests, and installer fixtures pass. Static metadata and native-code checks are described below. A startup log from the installed compatibility build confirms IL2CPP resolution, hook installation, and DirectX/menu initialization. Controlled gameplay and lifecycle acceptance checks were not completed; the in-game checks at the end remain outstanding. The broader reliability work is deferred and is not part of this compatibility release.

## Inputs and provenance

- Installed Steam manifest: app `1280770`, build `24496548`, depot `1280771`, manifest `4647145190882772546`.
- Public branch: `24496548`, independently compared with the [SteamCMD app information endpoint](https://api.steamcmd.net/v1/info/1280770). The beta branch is a separate target.
- `GameAssembly.dll` SHA-256: `467336b989f5145e004ed3730df3e1628bcf858fb125e9a50f84693a8eb465aa`.
- Matching `global-metadata.dat` SHA-256: `22e0b7bf32d6139bcb69eb9a43b412f6a2c7b1f3d865cbd8371215dafa78cd1f`.
- Fresh Il2CppDumper 6.7.46 output: metadata version 24.4, automatically selected native layout 24.5. The dump generated earlier in this task from that exact pair was reused; the July dump in this repository remains a historical reference.
- Prior evidence: the local July `reverse_docs` contracts, matching prior `dump.cs`/`script.json`, and the tracked [source-used contracts](contracts.md).
- Scope: the user requested updating this local project; analysis and source/build changes were local. The ignored `out/binding-update` directory contains the scope, comparison scripts, disassembly and build/test logs. Generated metadata and game binaries are not tracked.

## Native binding migration

This table records the historical old/new comparison. [`game_offsets.h`](../../src/core/game_offsets.h) remains authoritative for production values.

| Binding | Previous RVA | Verified RVA | Identity evidence |
| --- | --- | --- | --- |
| `fire_primary_shot` | `0x1814A80` | `0x110D0B0` | Same obfuscated metadata name and exact native signature; authentic discharge flow retained |
| `compute_weapon_spread` | `0x1804CD0` | `0x11059B0` | Same metadata name/signature; native full-sum spread arithmetic |
| `update_crosshair_spread` | `0x1808B30` | `0x1107A70` | Same metadata name/signature; ADS guard and all four transform writes |
| `physics_raycast` | `0x17D9740` | `0x10959A0` | Exact five-argument Vector3/Vector3 overload returning bool |
| `physics_raycast_all` | `0x17D84F0` | `0x1094750` | Exact five-argument Vector3/Vector3 overload returning RaycastHit array |

Complete native signatures, with only generated parameter names simplified:

```cpp
void FirePrimaryShot(PlayerController_o* self, ItemInfo_ShotInfo_o* shot, const MethodInfo* method);
void ComputeWeaponSpread(PlayerController_o* self, const MethodInfo* method);
void UpdateCrosshairSpreadLayout(PlayerController_o* self, const MethodInfo* method);
bool Raycast(UnityEngine_Vector3_o origin, UnityEngine_Vector3_o direction,
             float maxDistance, int32_t layerMask, int32_t queryTriggerInteraction,
             const MethodInfo* method);
UnityEngine_RaycastHit_array* RaycastAll(UnityEngine_Vector3_o origin, UnityEngine_Vector3_o direction,
                                      float maxDistance, int32_t layerMask, int32_t queryTriggerInteraction,
                                      const MethodInfo* method);
```

The three PlayerController names each matched exactly one new signature. Unity overloads were matched by their complete signature, not by name alone. The old and new PlayerController method counts differ, so declaration order was not used as binding evidence.

### Native behavior checks

- Primary fire still accesses the selected item and ShotInfo, updates the item cooldown, calls the verified spread function, loops over pellet count, sorts raycast hits, and retains ammo/result/recoil work. Its raycast call at RVA `0x110D645` enters the four-argument wrapper at `0x1094700`; that wrapper calls the five-argument hook target at `0x109473B`, supplying the default trigger policy. The existing thread-local primary-shot scope therefore still contains the intended query. Single-hit validation remains unhooked and NonAlloc remains unused.
- Spread calls `Rigidbody.get_velocity`, `Vector3.get_magnitude` and `Mathf.Clamp`, then selects hip/ADS base and multiplier. Native add/add/multiply instructions still implement `(speed + base + itemSpread) * multiplier` and write the controller's current spread. Existing call preconditions remain necessary.
- Crosshair refresh still returns early during ADS, checks the four arrays and style index, and writes the same directional local-position components through Transform getters/setters.

These checks carry forward the existing `CALL` and `HOOK_PRESERVE` contracts. No additional mutation, artificial shot invocation or changed original-call ordering was introduced.

## Field, metadata and ABI review

Every raw field in the production inventory was checked against the fresh declarations. None changed location or representation. The player declaration changed the unrelated gear pointer type name from `ODMGear` to `GrappleGear`; it did not shift the following fields.

| Category | Static result |
| --- | --- |
| Player inventory, selected index, ADS, shield, sprint, Rigidbody, movement and measured speed | Existing offsets/types retained; spread/crosshair use corroborated by native accesses |
| Player statistics | Same dictionary reference; `StatInfo.defaultValue`, `modifier`, `offset` remain available |
| Identity/name/team | `PlayerTeamOutline.player`, `identity`, `rend`, MyceliumIdentity owner reference, player-data name, TeamInfoManager/TeamInfo fields retain their layouts |
| Lifecycle and item metadata | `PlayerController.Update`, `LocalInstance`, `items`, `cam`, `shieldObject`, Item and ItemInfo/ShotInfo named members remain available |
| Rigidbody | Declared in `UnityEngine.PhysicsModule`, not `UnityEngine.CoreModule`; class and four accessor lookups corrected in `gameplay.cpp` |
| RaycastHit | Point, normal, face ID, distance, UV and collider instance ID match the local 44-byte value layout |
| IL2CPP objects and boxing | Two-pointer object header, followed by value payload, matches the local declarations |
| Arrays and strings | Array object/bounds/length/data prefix and UTF-16 length/character layout match the local declarations |
| Statistics dictionary | Buckets/entries/count/version/free-list/free-count prefix and hash/next/key/value entry match the local declarations |
| MethodInfo | Function pointer, invoker, name, class, return/parameter pointers, two union slots and trailing flags match the local declaration |
| Ammo | The two-integer representation remains offset/value; the native decoder subtracts the first word from the second |

The Rigidbody correction is part of binding resolution: looking up its class in CoreModule returns no class, preventing the velocity and gravity paths from resolving even with correct raw RVAs.

## Evidence and verification

| Evidence | Finding | Production path |
| --- | --- | --- |
| E-001: matching input hashes and public/installed build | F-001: correct current public target; high confidence, statically verified | Startup build comparison → raw binding eligibility |
| E-002: old/new complete signatures plus native disassembly | F-002: five relocated methods retain required ABI/data flow; high confidence, runtime pending | Local primary-shot scope → RaycastAll wrapper → hook target; spread restoration → original compute/refresh |
| E-003: current generated fields and native accesses | F-003: raw layouts retained; Rigidbody assembly corrected; high confidence, runtime pending | Metadata lookup → local movement/weapon/ESP paths |
| E-004: successful Release/CTest/installer output | F-004: source builds and existing portable checks pass; validated locally | Release build → tests → reviewable DLL |

Findings are compatibility observations (`n/a_re` severity). Input hashes provide content identity for E-001 through E-003; build/test output is reproducible rather than a binary compatibility proof.

Commands run from the repository root:

```bat
scripts\build_release.bat
ctest --test-dir build-release -C Release --output-on-failure
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\test_installer.ps1
git diff --check
```

Timeline: inspected the 1.2.0 source and July contracts → matched all five complete signatures → verified native call chains and field/ABI layouts → updated centralized bindings and Rigidbody lookups → passed release and portable/installer checks.

## Remaining in-game checks

The runtime checklist in [updating bindings](../updating-bindings.md) remains required before claiming gameplay validation: initialize with features disabled, confirm menu/resize/unload, test hip/ADS spread restoration, primary-shot and raycast scoping, ESP names/teams, movement/gravity, respawn/session changes, and final unload restoration. None of those live checks was performed during this update.
