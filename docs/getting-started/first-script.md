# Your first script

## Summary

The smallest working mod. You write a Luau file, then run it from your Geode mod's C++ with `runFile`.

## Step 1: write the script

Create a `.luau` file in your mod resources, for example `Bootstrap.luau`.

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
LuauAPI owns the runtime, so you do not start it. Check `status()` is `Ready` first if you need to.

## The rules

The file name must be a flat `.luau` resource name inside the resources directory you pass.
See [modules](../reference/lua/modules.md) and [Limits and errors](../reference/cpp/limits-and-errors.md).

## Developer mode

LuauAPI ships built-in developer tools, such as a script executor.
They load only when developer mode is on in mod settings.
See [Installation](installation.md).

## Next

- [Editor setup](editor-setup.md)
- [Examples](examples.md)

## Related

- [Getting started](overview.md)
- [Examples](examples.md)
- [globals](../reference/lua/globals.md)
- [modules](../reference/lua/modules.md)
- [C++ integration guide](../reference/cpp/integration-guide.md)

## Source

- `src/api.cpp`
- `include/LuauAPI.hpp`
- `src/core/Runtime.cpp`
