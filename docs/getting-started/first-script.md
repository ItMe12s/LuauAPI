# Your first script

## Summary

The smallest working mod. You write a Luau file, then run it from your Geode mod's C++ with `runFile`.

The [example mod template](https://github.com/ItMe12s/luauapi-example-mod)
already contains the matching `src/main.cpp` and `mod/Bootstrap.luau`.
Follow this page when adding the same setup to an existing mod.

## Step 1: write the script

Create a `.luau` file in your mod resources, for example `Bootstrap.luau`.
List it in your `mod.json` resources. See [Installation](installation.md).

```lua
print("Hello from Luau")
```

See [globals](../reference/lua/globals.md) for `print` behavior.

## Step 2: run it from C++

Include the header and call `runFile` with your resources directory and the file name.
All public functions live in the `imes::luauapi` namespace.

LuauAPI exports `include/LuauAPI.hpp` through `api.include` in its `mod.json`.

```cpp
#include <Geode/Geode.hpp>
#include <Geode/loader/ModEvent.hpp>
#include <imes.luauapi/include/LuauAPI.hpp>

using namespace geode::prelude;
namespace lua = imes::luauapi;

$on_mod(Loaded) {
    auto result = lua::runFile(Mod::get()->getResourcesDir(), "Bootstrap.luau");
    if (result.isErr()) {
        log::error("script failed: {}", result.unwrapErr());
    }
}
```

See [Getting started](overview.md) for the main-thread rule.
`$on_mod(Loaded)` already runs on the main thread, so call `runFile` directly there.
LuauAPI owns the runtime, so you do not start it.
Check `imes::luauapi::status()` returns `RuntimeStatus::Ready` if you need to.
If `Bootstrap.luau` needs functions or values from your C++ code, register them before `runFile`.

Run the mod. The Geode console shows `Hello from Luau`.

## The rules

The file name must be a flat `.luau` resource name inside the resources directory you pass.
See [Installation](installation.md) for how Geode packs resource files.
Path and size rules live in [modules](../reference/lua/modules.md) and [Limits and errors](../reference/cpp/limits-and-errors.md).

## Using the executor

LuauAPI includes a built-in script executor. It is an ImGui window where you write Luau and run it live in the game.
Turn on **Enable Developer Mode** under Developer Settings in LuauAPI mod settings, restart the game, then turn on **Enable Script Executor**.
The executor toggle appears only after developer mode is on.

## Next

- [modules](../reference/lua/modules.md)

## Related

- [Getting started](overview.md)
- [Editor setup](editor-setup.md)
- [globals](../reference/lua/globals.md)
- [tasks and time](../reference/lua/tasks.md)
- [C++ API reference](../reference/cpp/api-reference.md)

## Source

- `src/api.cpp`
- `include/LuauAPI.hpp`
- `src/core/Runtime.cpp`
