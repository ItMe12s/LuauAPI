# Platform parity

## Summary

Codegen produces a single Luau stub and C++ binding set,
but GD uses different binding addresses per platform.
This explains how the generator keeps cross-platform safety and how to read the parity report when symbols are missing.

## Forced intersection

Binding decisions run per platform, then intersect.

Per-platform plan:

- In `emit/plan.py`, the function `collect_platform_plan(root, platform)` runs `group_supported()` for a specific platform.
- A method is included if it is callable on that platform, meaning it has a real address (or is marked `link` / `inline`),
  its arguments and return type can be marshalled, and it is not denied.

Intersection platforms:

- The main platforms are `win`, `m1` (or `imac`), `ios`, `android32`, and `android64`.
- These are defined in `policy/intersection.py` as `intersection_platforms`.

Forced intersection:

- `collect_plan()` calls `_apply_intersection()`.
- A method is kept only if it is supported on every intersection platform.
- If a method is supported on some but not all platforms,
  it is dropped with the reason `intersection-missing-platform:<platforms>`.
- This rule applies to hooks, fields, and namespace free functions as well.
- Classes with no members left after intersection become type-only stubs or are skipped entirely.

The result is one common surface in generated bindings and `types/geode.d.luau`.
If the stub type-checks a call, that call is valid on all supported platforms.
Platform-only APIs never reach Lua authors.

iOS is strict (`STRICT_DIRECT_PLATFORMS = {"ios"}`).
On iOS a method needs a hard address, `link`, or `inline`, with no soft fallback,
so iOS is usually the platform that drops a symbol from the intersection.

## Parity report

`emit/parity.py` writes `parity.json` (always) and a markdown report (with `--parity-report-out`).

For each method key (`Class.method(argtypes)`), the report lists:

- `supportedPlatforms`
- `hookablePlatforms`
- `hookAddressMissingPlatforms`
- `skipReasons`

The `summary` block has per-platform counts.
The `freeFunctions` block records the same shape plus `finalSkipReason`.

The `intersection` block lists:

- Final common counts
- Number of methods removed by intersection
- Number of hooks removed by intersection
- Number of fields removed by intersection
- Number of free functions removed by intersection

`hints` includes the following gap categories to address:

- `winMissingCallableProof`
- `iosSkippedClasses`
- `hookAddressGaps`
- `macGapReasons`

## Platform-scoped fields

Broma bindings may wrap members in platform blocks.
The parser attaches a `platforms` set to such fields. Per-platform plans only consider fields that apply on that platform,
and the intersected stub omits platform-only members entirely.

## Policy

- No per-symbol platform stubs. A symbol missing on some platforms is dropped, not shimmed.
  The type checker is the contract.
- Only the intersected `types/geode.d.luau` is emitted, to keep mods cross-platform.
- To close a gap, add a verified address or `link` metadata, not by relaxing filters.
  The symbol reappears automatically.

## Related

- [Codegen](codegen.md)
- [Testing](../testing.md)

## Source

- `tools/luau_codegen/emit/plan.py`
- `tools/luau_codegen/policy/intersection.py`
- `tools/luau_codegen/policy/filtering.py`
- `tools/luau_codegen/emit/parity.py`
