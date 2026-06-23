# Changelog

[Check the GitHub commits for more details](https://github.com/ItMe12s/LuauAPI).

## v0.1.0-beta.9

- Added crash sidecar that writes `luauapi-last-context.txt` with Luau context before native faults.
- Updated `print`, `warn`, and panic to log with the owning mod id instead of `[lua]`.
- Fixed crash in the global `CCObject::release` hook when releasing untracked objects during teardown.

## v0.1.0-beta.8

- Switched `geode.fields` cleanup to Geode type casts instead of pointer reinterpretation.
- Switched viewport and ccCArray view element casting to Geode type casts.
- Fixed CCObject casting for callback trampoline anchor binding.

## v0.1.0-beta.7

- Redesigned deferred CCObject release drain for cross-platform stability.
- Fixed `geode.fields` throwing `unordered_map::at: key not found`.
- Fixed dynamic node/object userdata losing subtype methods.
- Fixed crash when spamming menu callbacks.

## v0.1.0-beta.6

- Added global `warn` for script logging at warn level.
- Fixed macOS crash in ImGui render when scene changes disturbed GL state.
- Fixed crash in the global `CCObject::release` hook when releasing objects with no Luau field or trampoline state.
- Fixed macOS crash in deferred CCObject release drain when final release evicted schedule/menu trampolines during CCScheduler update.
- Removed `:retain()` and `:release()` bindings on game objects.

## v0.1.0-beta.5

- Added Luau read/write for encrypted stat fields (SeedValue) as plain numbers.
- Improved cleanup when the mod unloads for `geode.utils.web`, keyboard listeners, `websocket`, and `imgui.onDraw`.
- Updated runtime internals for bindings, memory limits, and module loading.
- Fixed crash in deferred CCObject release drain when an owned object is freed before the next tick.

## v0.1.0-beta.4

- Added user settings for ImGui scaling.
- Added ImGui font API (`imgui.font`).
- Added disabling all GPU features/APIs when changing Fullscreen/Windowed mode (temporary *"fix"*).
- Switched 3D viewport backend from CCNode to CCSprite.
- Updated default executor font.
- Fixed crashes when garbage-collecting node/object userdata.
- Fixed tiny ImGui on macOS Retina with improved scaling and fonts (forked matcool/gd-imgui-cocos).
- Fixed crash in deferred CCObject release drain when GC queues the same object as both borrowed and owned userdata.

## v0.1.0-beta.3

- Switched to MIT license.
- Updated default executor theme.
- Fixed dynamic userdata typing for `CCNode*` and `CCObject*` returns.

## v0.1.0-beta.2

- Updated demo scripts (`_viewportdemo.luau`, `_helloworlddemo.luau`) and examples with proper Z order.
- Fixed crash when `imgui.onDraw` runs before OpenGL is ready on macOS and other non-Windows platforms.
- Fixed Luau hooks on Intel macOS (`imac`) builds.
- Fixed "luau runtime accessed off main thread" errors on macOS.
- Fixed "GLSL 110 does not allow sub- or super-matrix constructors" errors on macOS.

## v0.1.0-beta.1

- Initial beta release
