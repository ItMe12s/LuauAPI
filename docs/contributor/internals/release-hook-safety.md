# Release-hook safety

## Summary

LuauAPI installs a global hook on `cocos2d::CCObject::release`.
That hook fires for every CCObject released anywhere, including during Cocos2d destructor cascades.
This page states the rule that keeps that hook (and the deferred release drain) safe.

Code that breaks this rule has caused most of the native crashes this mod has shipped.
Read this before you change any function reachable from `release` or the drain.

## The rule

Any function reachable from `CCObject::release` or the deferred release drain must do a non-virtual membership check first.
No virtual call, no RTTI cast, no map `at`, no `retainCount`, nothing that dereferences or dispatches on the object,
until a map lookup confirms the object is tracked. If the lookup misses, return before touching the object.

## Why

During scene-graph teardown Cocos2d runs destructor cascades.
A node destructor clears member containers, which call `release` on every element.
Those elements are mostly objects LuauAPI never tracked: rings, particles, portals, dictionary entries, and so on.
Some are mid-destruction by the time the hook sees them.

A virtual call on a mid-destruction object is not safe.
`geode::cast::typeinfo_cast` walks typeinfo, and that walk has faulted on a half-dead object during this exact cascade.
That was the June 2026 crash fixed in `v0.1.0-beta.9`.

Before the hook runs, most objects are untracked.
The map `find` only hashes and compares the address value.
It never dereferences the object, so it is safe on a half-dead pointer and costs one hash miss for the common case.
Virtual work must wait until after that miss.

## What counts as a virtual call

Treat any of these as virtual work and keep them out of the pre-check path:

- `geode::cast::typeinfo_cast` and `dynamic_cast`
- any method call on the object, including `retainCount` and `release`
- `geode::WeakRef` construction from the raw object
- `unordered_map::at`, which throws instead of returning not-found
- any read of object members

A `static_cast` or `reinterpret_cast` used only as a map key is fine.
It changes how the compiler views the address, it does not read or call the object.
The map `find` then either hits (object is tracked) or misses (object is not), and only a hit reaches the virtual work.

## Where the hook lives

Codegen emits the hook body and installs it.

The generated body (in `tools/luau_codegen/emit/bindings/common.py`) is:

```cpp
void luaapi_fields_release_hook(cocos2d::CCObject* self) {
    luax::Fields::evictIfFinalRelease(self);
    luax::evictTrampolinesIfFinalRelease(self);
    luax::dropBorrowedTargetIfFinalRelease(self);
    self->release();
}
```

Codegen resolves the `CCObject::release` address per platform and hooks it with `geode::Mod::get()->hook`.
A failed resolve, install, or enable is non-fatal.
The hook is skipped and the mod logs a warning, so a bad binding address never blocks the mod from loading.

All three `evictIfFinalRelease` calls must obey this rule because all three run on every release.

## The compliant functions

`Fields::evictIfFinalRelease` in `src/framework/usertype/Fields.cpp` is the model:

```cpp
void Fields::evictIfFinalRelease(cocos2d::CCObject* object) {
    if (!object) return;
    auto& tables = fieldTables();
    auto it = tables.find(static_cast<cocos2d::CCNode*>(static_cast<void*>(object)));
    if (it == tables.end() || !entryStillOwnsNode(it->second, it->first)) return;
    if (object->retainCount() > 1) return;
    evict(it->first);
}
```

The `static_cast` through `void*` is intentional and safe.
The map is keyed by the real `CCNode*` handed to `Fields::push`,
so for an untracked object the `find` misses and the cast result is never dereferenced.
`entryStillOwnsNode` re-checks the weak owner before any further work.
`retainCount` is called only after the map confirms the object is one of ours.

`evictTrampolinesIfFinalRelease` in `src/framework/callback/LuaTrampolineRegistry.cpp` follows the same shape.
It does its own `anchorMap().find(anchor)` and returns on a miss before any call on the anchor.

`dropBorrowedTargetIfFinalRelease` in `src/framework/usertype/Usertype.cpp` follows the same shape.
It does its own `borrowedTargets().find(object)` and returns on a miss before `retainCount`.
When a borrowed userdata target reaches its final release, the set entry is removed before Geode `WeakRef` pool cleanup can run.
`liveObject` then skips `WeakRef::lock` for that target and returns `nullptr` instead of faulting.

## What the drain must do too

The deferred release drain in `src/framework/usertype/DeferredRelease.cpp`
runs every frame from the `CCDirector::drawScene` hook in `src/bindings/task/TaskSchedulerDrainHook.cpp`.
It calls `release` on owned objects it has held onto.

A release inside the drain re-enters the global hook.
So the drain inherits the same risk: every object it releases is one more pass through the hook body.
The drain mitigates this by parking weak refs for pool safety (`WeakRefShutdown.hpp`)
and by checking `Runtime::isShuttingDown()` to short-circuit during teardown.
Any new drain logic must assume the released object is already being destroyed and must not inspect it after release.

## When you can use a cast freely

`Fields::evict(cocos2d::CCObject*)` is a different entry point. It is not on the release cascade.
A caller hands it one object on purpose, so it is safe to `typeinfo_cast` to `CCNode*` there and return on a null cast.
That function keeps its cast, and the guard test requires it.

The rule is path-specific.
The question is not "can this function cast" but "can this function be reached from `release` while the object is half-dead".
If yes, gate on a membership check first. If no, cast normally.

## How this is enforced

Two layers keep the rule alive after this page:

- A Python guard test in `tests/luau_codegen/guards/test_binding_guards_framework.py`
  checks that `evictIfFinalRelease` does not use `typeinfo_cast` and goes through `fieldTables()` and `entryStillOwnsNode`,
  that `dropBorrowedTargetIfFinalRelease` checks `borrowedTargets()` before `retainCount`,
  while `evict(CCObject*)` still does use `typeinfo_cast`.
- Host tests in `tests/cpp/framework/fields_tests.cpp` cover the no-op case for an untracked node and for a non-node CCObject.
- Host tests in `tests/cpp/framework/usertype_tests.cpp` cover borrowed userdata access after `dropBorrowedTargetIfFinalRelease`.

If you add work to a release-reachable function, add a test that runs it on an untracked object and on a non-node CCObject.
Extend the guard test so the next change cannot quietly put a virtual call back on the path.

## Related

- [Bindings framework](bindings-framework.md)
- [Runtime](runtime.md)
- [Crash sidecar](crash-sidecar.md)
- [Architecture](../architecture.md)
- [Codegen](../codegen/codegen.md)
- [Limits and errors](../../reference/cpp/limits-and-errors.md)
- [game objects](../../reference/lua/game-objects.md)

## Source

- `src/framework/usertype/Fields.cpp`
- `src/framework/usertype/Usertype.cpp`
- `src/framework/callback/LuaTrampolineRegistry.cpp`
- `src/framework/usertype/DeferredRelease.cpp`
- `src/framework/usertype/WeakRefShutdown.hpp`
- `src/bindings/task/TaskSchedulerDrainHook.cpp`
- `tools/luau_codegen/emit/bindings/common.py`
- `tests/cpp/framework/fields_tests.cpp`
- `tests/cpp/framework/usertype_tests.cpp`
- `tests/luau_codegen/guards/test_binding_guards_framework.py`
