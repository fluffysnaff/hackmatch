# Production binding inventory

Supported game build: `23904900`. Current values live in [`game_offsets.h`](../../src/core/game_offsets.h).

## Native methods

| RVA | Recovered function | Production use | Evidence | Mode |
| ---: | --- | --- | --- | --- |
| `0x1814A80` | `PlayerController.FirePrimaryShot(PlayerController*, ShotInfo*, MethodInfo*)` | Scope eligible local primary-shot raycasts while preserving the original call | Primary-fire contract and decompile | `HOOK_PRESERVE` |
| `0x1804CD0` | `PlayerController.ComputeWeaponSpread(PlayerController*, MethodInfo*)` | Recompute spread after restoring item values | Exact arithmetic and callers | `CALL` |
| `0x1808B30` | `PlayerController.UpdateCrosshairSpreadLayout(PlayerController*, MethodInfo*)` | Refresh non-ADS crosshair presentation | Four transform-array writes | `CALL` |
| `0x17D9740` | Unity `Physics.Raycast(Vector3, Vector3, float, int, int, MethodInfo*)` | Validate a redirected shot through the saved original | Unity binding and runtime signature; no PlayerController body | `CALL` |
| `0x17D84F0` | Unity `Physics.RaycastAll(Vector3, Vector3, float, int, int, MethodInfo*)` | Redirect direction only inside a local primary shot | Call site in primary-fire body and runtime signature | `HOOK_PRESERVE` |
| metadata | `PlayerController.Update(PlayerController*, MethodInfo*)` | Local per-frame feature integration | Resolved by metadata name; lifecycle contract | `HOOK_PRESERVE` |

The Unity raycast RVAs and exact `RaycastHit` layout require runtime validation after every game update; the PlayerController decompile proves their role but not Unity ABI stability.

## Raw fields

| Owner | Offset | Type / meaning | Feature | Evidence |
| --- | ---: | --- | --- | --- |
| `PlayerTeamOutline.identity` | `0x20` | player-data pointer | ESP name/team snapshot | Runtime-observed chain; weak static evidence |
| player data | `0x20` | `Il2CppString*` name | ESP labels | Runtime-observed chain; weak static evidence |
| `PlayerController` | `0x60` | `Item*[]` | spread and selected item | Start, Update, shot, spread bodies |
| `PlayerController` | `0x108` | selected item index | spread and selected item | Start, Update, shot, spread bodies |
| `PlayerController` | `0x10C` | paired shield/status bool | target filter and local shield disable | shield activation/deactivation bodies; gameplay authority unresolved |
| `PlayerController` | `0x110` | stats dictionary | gravity/stat restoration | Start, Update, FixedUpdate; individual key semantics need runtime checks |
| `PlayerController` | `0x11C` | smoothed local movement intent | collision-preserving speed | Update producer and FixedUpdate consumer |
| `PlayerController` | `0x161` | ADS bool | spread and diagnostics | Update, shot, spread, crosshair bodies |
| `PlayerController` | `0x164` | sprint state | auto sprint and diagnostics | Update plus set/clear helpers |
| `PlayerController` | `0x98/0xA0/0xA8/0xB0` | crosshair transform arrays | spread presentation | crosshair layout body |
| `PlayerController` | `0x180` | `Rigidbody*` | spread, movement, diagnostics | lifecycle, Update, FixedUpdate, shot bodies |
| `PlayerController` | `0x208` | crosshair style index | spread presentation | crosshair layout body |
| `PlayerController` | `0x280` | measured fixed-step speed | Movement Graph | FixedUpdate displacement calculation |
| `Item` | `0x18` | item-definition pointer | spread precondition | shot and spread bodies |

## Runtime ABI assumptions

- IL2CPP boxed values start after `il2cpp::Object`; arrays use the local `il2cpp::Array::vector` declaration.
- Managed strings use the local object, length, and UTF-16 character layout.
- Stats traversal depends on the matching IL2CPP dictionary-entry layout.
- Aim hit scanning depends on the matching Unity `RaycastHit` layout, including collider instance ID.
- The metadata-resolved update hook reads `MethodInfo::methodPointer` from the local declaration.

These assumptions are bindings even though they are represented as C++ layouts rather than constants. Validate them with matching metadata and controlled runtime observation before changing the supported build.
