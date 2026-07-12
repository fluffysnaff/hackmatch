# Game reverse-engineering notes

This directory is the compact, tracked evidence set for raw Redmatch 2 bindings used by Hackmatch. It covers Steam build `23904900`, Windows x64, `GameAssembly.dll` image base `0x180000000`.

The full generated research workspace is intentionally ignored. When a local `reverse_docs/README.md` exists, use its more complete contracts and raw output first. Otherwise use this directory in this order:

1. [`bindings.md`](bindings.md) for every production RVA and raw field offset.
2. [`contracts.md`](contracts.md) for integration constraints and confidence.
3. [`raw-decompiles.md`](raw-decompiles.md) for compact C-like reconstructions of the relevant data flow.

`src/core/game_offsets.h` is the production authority for numeric values. These notes explain why a value exists; they must not become a second source of constants.

## Address rules

- RVA is relative to the module image base.
- VA is image base plus RVA.
- A file offset is not interchangeable with either value.
- Stable IL2CPP members are resolved by metadata name and are not duplicated here.
- Raw hooks and raw-layout ESP paths fail closed when the installed Steam build differs from `supported_build`.

## Integration modes

- `BIND`: read or write a verified field in its documented owner and lifecycle.
- `CALL`: invoke the original function only with its authentic preconditions.
- `HOOK_PRESERVE`: detour while always preserving original execution, ABI, ordering, and ownership gates.
- `OBSERVE_ONLY`: evidence is useful for understanding but insufficient for mutation.
- `REJECT`: known unsafe or semantically unsupported use.

All persistent writes must be snapshotted and restored on disable, player replacement, session loss, and unload.
