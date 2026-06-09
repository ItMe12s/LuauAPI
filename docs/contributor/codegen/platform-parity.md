# Platform parity

## Summary

Codegen produces a single Luau stub and C++ binding set, but GD uses different binding addresses per
platform. This explains how the generator keeps cross-platform safety and how to read the parity
report when symbols are missing.

## Forced intersection

Binding decisions run per platform, then intersect.

- Per-platform plan. `emit/plan.py` `collect_platform_plan(root, platform)` runs `group_supported()`
  for one platform. A method stays only if it is callable there: it has a real address (or `link` /
  `inline`), its args and return marshal, and it is not denied.
- Intersection platforms are `win`, `m1` (or `imac`), `ios`, `android32`, `android64`
  (`policy/intersection.py` `intersection_platforms`).
- Forced intersection. `collect_plan()` calls `_apply_intersection()`, which keeps a method only when
  every intersection platform supports it. Anything supported on some but not all is dropped with
  reason `intersection-missing-platform:<platforms>`. Hooks, fields, and namespace free functions use
  the same rule. Classes with no members left become type-only stubs or are skipped.

The result is one common surface in generated bindings and `types/geode.d.luau`. If the stub
type-checks a call, that call is valid on all supported platforms. Platform-only APIs never reach Lua
authors.

iOS is strict (`STRICT_DIRECT_PLATFORMS = {"ios"}`). On iOS a method needs a hard address, `link`, or
`inline`, with no soft fallback, so iOS is usually the platform that drops a symbol from the
intersection.

## Parity report

`emit/parity.py` writes `parity.json` (always) and a markdown report (with `--parity-report-out`).
Per method key (`Class.method(argtypes)`) it records `supportedPlatforms`, `hookablePlatforms`,
`hookAddressMissingPlatforms`, and `skipReasons`. The `summary` block has per-platform counts. The
`freeFunctions` block records the same shape plus `finalSkipReason`. The `intersection` block has
final common counts and how many methods, hooks, fields, and free functions intersection removed.

`hints` lists gaps to fix: `winMissingCallableProof`, `iosSkippedClasses`, `hookAddressGaps`, and
`macGapReasons`.

## Platform-scoped fields

Broma bindings may wrap members in platform blocks such as `android, ios { int m_spawnCount; }`. The
parser attaches a `platforms` set to such fields. Per-platform plans only consider fields that apply
on that platform, and the intersected stub omits platform-only members entirely.

## Policy

- No per-symbol platform stubs. A symbol missing on some platforms is dropped, not shimmed. The type
  checker is the contract.
- Only the intersected `types/geode.d.luau` is emitted, to keep mods cross-platform.
- To close a gap, add a verified address or `link` metadata, not by relaxing filters. The symbol
  reappears automatically.

## Related

- [Codegen](codegen.md)
- [Testing](../testing.md)

## Source

- `tools/luau_codegen/emit/plan.py`
- `tools/luau_codegen/policy/intersection.py`
- `tools/luau_codegen/policy/filtering.py`
- `tools/luau_codegen/emit/parity.py`
