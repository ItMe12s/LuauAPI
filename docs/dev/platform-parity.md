# Platform parity

## Summary

Codegen produces a single Luau stub and C++ binding set, but GD uses different binding addresses per platform.
This explains how the generator ensures cross-platform safety and how to read the parity report when symbols are missing.

## Forced intersection

Binding decisions run per platform, then intersect.

**Per-platform plan.** `emit/plan.py` `collect_platform_plan(root, platform)` runs `policy/filtering.py` `group_supported()` for one platform.

A method stays only if it is callable there:

- It has a real address (or `link` / `inline`)
- Its args and return marshal, and it is not denied.

Intersection platforms are `win`, `m1` (or `imac`), `ios`, `android32`, `android64` (`policy/intersection.py` `intersection_platforms`).

**Forced intersection.** `collect_plan()` calls `_apply_intersection()`, which keeps a method only when every intersection platform supports it.
Anything supported on some but not all is dropped with reason `intersection-missing-platform:<platforms>`.
Hooks and fields use the same rule. Classes with no members left are skipped, or become type-only stubs (see `emit/luau_types/references.py`).

The result is one common surface in `geode.d.luau`. If the stub type-checks a call, that call is valid on all supported platforms.
Platform-only APIs never reach Lua authors.

**iOS is strict.** `STRICT_DIRECT_PLATFORMS = {"ios"}` in `filtering.py`.
On iOS a method needs a hard address, `link`, or `inline` (no soft fallback), and classes without callable proof are pruned.
iOS is usually the platform that drops a symbol from the intersection.

## Parity report

`emit/parity.py` writes `parity.json` (always) and a markdown report (with `--parity-report-out`).

Per method key (`Class.method(argtypes)`) it records:

- `supportedPlatforms`: platforms where the method binds.
- `hookablePlatforms`: platforms where a `before`/`after` hook can attach.
- `hookAddressMissingPlatforms`: callable but no hook address on a supported platform.
- `skipReasons`: per platform, why it was dropped (`missing-address`, `not-callable:<p>`, `intersection-missing-platform:...`, `inaccessible`, and others).

The `summary` block has per-platform counts (methods emitted/skipped, hook targets, fields, skipped classes).
The `intersection` block has final common counts and how many methods, hooks, and fields intersection removed.

### Hints

`hints` lists gaps to fix:

- `winMissingCallableProof`: method skipped on `win` for missing/callable proof but supported elsewhere. Only add a verified `win` offset or `link(win)`.
- `iosSkippedClasses`: classes iOS pruned for lack of callable proof.
- `hookAddressGaps`: bindable methods missing a hook address on at least one supported platform.
- `macGapReasons`: why `m1`/`imac` lags `android64`.

## Policy

- No per-symbol platform stubs. A symbol missing on some platforms is dropped, not emitted with a runtime shim. The type checker is the contract.
- No platform-specific stubs. Only the intersected `geode.d.luau` is emitted to keep mods cross-platform.
- To close a gap, add a verified address or `link` metadata (not by relaxing filters). The symbol will reappear automatically.

## Source

- `tools/luau_codegen/emit/plan.py`
- `tools/luau_codegen/policy/intersection.py`
- `tools/luau_codegen/policy/filtering.py`
- `tools/luau_codegen/emit/parity.py`
