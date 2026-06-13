# Your first script

## Summary

The smallest working mod. You write a Luau file, then run it from your Geode mod's C++ with `runFile`.

## Step 1: write the script

Create a `.luau` file in your mod resources, for example `Bootstrap.luau`.

```lua
print("Hello from Luau")
```

See [Globals](../reference/lua/globals.md) for `print` behavior.

## Step 2: run it from C++

Include the header and call `runFile` with your resources directory and the file name.
All public functions live in the `imes::luauapi` namespace.

```cpp
#include <Geode/Geode.hpp>
#include <Geode/loader/ModEvent.hpp>
#include <imes.luauapi/include/LuauAPI.hpp>

using namespace geode::prelude;
namespace lua = imes::luauapi;

$on_mod(Loaded) {
    queueInMainThread([] {
        auto result = lua::runFile(Mod::get()->getResourcesDir(), "Bootstrap.luau");
        if (result.isErr()) {
            log::error("script failed: {}", result.unwrapErr());
        }
    });
}
```

`runFile` must run on the main thread. The example uses `queueInMainThread` to make sure of that.
LuauAPI owns the runtime, so you do not start it. Check `status()` is `Ready` first if you need to.

## The rules

The file name must be a flat `.luau` resource name inside the resources directory you pass.
See [Modules](../reference/lua/modules.md) and [Limits and errors](../reference/cpp/limits-and-errors.md).

## Developer mode

LuauAPI ships built-in developer tools, such as a script executor.
They load only when developer mode is on in mod settings.
See [Installation Settings](installation.md).

## Next

- [Editor setup](editor-setup.md)
- [Examples](examples.md)

## Related

- [Lua reference](../reference/lua/globals.md)
- [C++ integration guide](../reference/cpp/integration-guide.md)

## Source

- `src/api.cpp`
- `include/LuauAPI.hpp`
- `src/core/Runtime.cpp`
