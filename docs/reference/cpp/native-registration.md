# Native C++ registration

## Summary

Any Geode mod that depends on LuauAPI can expose typed C++ functions and values to Luau.
The mod's own scripts can call them. Other mods can use the same table as a shared API.
Registration is direct and does not expose the Luau stack to the calling mod.

## Setup

Declare LuauAPI as a required Geode dependency.
Include `LuauAPI.hpp` from the dependency's exported include directory.
See [Installation](../../getting-started/installation.md) for dependency setup.

A required mod dependency can expose a linked C++ API through its public headers.
Register a compatible free or static API function directly.
For an unsupported signature, register a free or static wrapper in your mod that calls the dependency.
Both forms still publish under the registering mod's id.

Registration must run on the main thread after the Luau runtime is ready.
It returns `Err` before readiness, during shutdown, or off the main thread.
It does not create the runtime or queue work for later.

Register functions and values before running a script that needs them:

```cpp
#include <Geode/Geode.hpp>
#include <Geode/loader/ModEvent.hpp>
#include <cmath>
#include <imes.luauapi/include/LuauAPI.hpp>

using namespace geode::prelude;
namespace lua = imes::luauapi;

bool isEven(int value) noexcept {
    return value % 2 == 0;
}

geode::Result<double> divide(double numerator, double denominator) {
    if (denominator == 0.0) return geode::Err("denominator must not be zero");

    auto quotient = numerator / denominator;
    if (!std::isfinite(quotient)) return geode::Err("quotient must be finite");

    return geode::Ok(quotient);
}

$on_mod(Loaded) {
    if (auto result = lua::registerFunction("math.isEven", &isEven); result.isErr()) {
        log::error("math.isEven registration failed: {}", result.unwrapErr());
        return;
    }

    if (auto result = lua::registerFunction("math.divide", &divide); result.isErr()) {
        log::error("math.divide registration failed: {}", result.unwrapErr());
        return;
    }

    if (auto result = lua::registerValue("math.defaultDivisor", 2); result.isErr()) {
        log::error("math.defaultDivisor registration failed: {}", result.unwrapErr());
        return;
    }

    if (auto result = lua::runFile(Mod::get()->getResourcesDir(), "Bootstrap.luau"); result.isErr()) {
        log::error("script failed: {}", result.unwrapErr());
    }
}
```

The mod's `Bootstrap.luau` can call its C++ code through its own id:

```lua
local Native = _G[geode.Mod.getID()]
print(Native.math.isEven(6))
print(Native.math.divide(8, Native.math.defaultDivisor))

local ok, err = pcall(Native.math.divide, 1, 0)
if not ok then
    print(err)
end
```

## Handling registration results

Each registration returns `geode::Result<void>`.
To use a local macro, replace the handler above with this version:

```cpp
#define LUAUAPI_REGISTER_OR_RETURN(expression)                                \
    do {                                                                      \
        auto result = (expression);                                           \
        if (result.isErr()) {                                                 \
            log::error("native registration failed: {}", result.unwrapErr()); \
            return;                                                           \
        }                                                                     \
    } while (false)

$on_mod(Loaded) {
    LUAUAPI_REGISTER_OR_RETURN(lua::registerFunction("math.isEven", &isEven));
    LUAUAPI_REGISTER_OR_RETURN(lua::registerFunction("math.divide", &divide));
    LUAUAPI_REGISTER_OR_RETURN(lua::registerValue("math.defaultDivisor", 2));

    if (auto result = lua::runFile(Mod::get()->getResourcesDir(), "Bootstrap.luau"); result.isErr()) {
        log::error("script failed: {}", result.unwrapErr());
    }
}

#undef LUAUAPI_REGISTER_OR_RETURN
```

This macro returns from a `void` setup handler on the first error.
Change its control flow before using it in a function with another return type.

> For quick prototyping, `(void)lua::registerFunction("math.isEven", &isEven);` explicitly ignores the result.
> This can hide why a value is missing from Luau. Do not use it when a script depends on the registration.

## Published layout

The caller-local `geode::Mod::get()` supplies the calling mod id.
LuauAPI does not resolve that id inside its own binary.

For a mod named `provider.mod`, registration publishes:

```lua
_G["provider.mod"].math.isEven(67)
_G["provider.mod"].math.divide(16, 4)
_G["provider.mod"].math.defaultDivisor
```

The mod's own scripts and scripts from other mods see this same table.
The full mod id is one `_G` key, including dots and dashes.
Only the registration path is split on dots.

Paths must be nonempty and cannot contain empty segments or embedded NUL bytes.
Each segment is a raw table key.
Names that do not work after a dot must use bracket syntax in Luau.

Missing tables are created and existing tables are reused.
A non-table intermediate or non-`nil` leaf returns `Err`.
Registration never replaces an existing leaf.
Failed registration leaves the Lua stack and visible globals unchanged.

## Public API

```cpp
template <NativeFunctionPointer Fn>
geode::Result<void> registerFunction(std::string_view path, Fn function);

template <NativeValue T>
geode::Result<void> registerValue(std::string_view path, T&& value);
```

Both functions copy required path and string data before entering the VM.
Always check the returned `geode::Result<void>`.
See [Limits and errors](limits-and-errors.md) for exact error strings.

## Registered values

`registerValue` supports these values:

| C++ value | Luau value | Rules |
| --- | --- | --- |
| `bool` | `boolean` | No truthiness conversion |
| Integral types and enums | `number` or `integer` | Must fit in signed 64 bits |
| `float`, `double` | `number` | Must be finite and range safe |
| `std::string`, `std::string_view` | `string` | Embedded NUL bytes are preserved |
| Character arrays and C strings | `string` | A null C string returns `Err` |
| `std::optional<T>` | inner value | Must be engaged and contain a supported scalar |

The direct types `wchar_t`, `char8_t`, `char16_t`, and `char32_t`
are rejected as registered values, function parameters, and return values.
Unsigned values above `INT64_MAX` return `Err`.
Small integral values use normal Luau numbers.
Values outside the exact number range use Luau's signed 64-bit integer value.
Published strings are copied into Luau.

## Function parameters

Free and static function pointers are supported, including `noexcept` functions.
Parameters may be supported scalars, optionals, or const references to those types.

| C++ parameter | Input rule |
| --- | --- |
| `bool` | Requires a Luau boolean |
| Integral type or enum | Requires an integral, destination-range-safe value |
| `float`, `double` | Requires a finite, range-safe value |
| `std::string` | Receives owned bytes |
| `std::string_view` | Borrows Lua bytes until the callback returns |
| `std::optional<T>` | Accepts the inner value or explicit `nil` |

Pointers, mutable references, rvalue references, member functions, captured callables,
`std::function`, nested optionals, and C-string parameters are rejected.

Arity is strict. The maximum is the declared parameter count. The minimum ends at the last required parameter.
Omitted arguments work only when every remaining parameter is optional.
A non-trailing optional needs an explicit value or `nil` when a required parameter follows it.
Colon calls add a `self` argument and fail unless the signature accepts that argument.

## Choosing a callback return

Use a supported return type other than `geode::Result<R>` when the callback does not need to report a recoverable failure.
Use `geode::Result<R>` when the callback can reject a domain value or report a recoverable failure.
`geode::Ok` produces the normal Luau return values.
`geode::Err` raises a Luau error that names the registered target.
Without `pcall`, the error propagates through the current Luau call.
With `pcall`, the first result is `false` and the second is the error value.

The callback result is created when Luau invokes the registered function.
It is separate from the `geode::Result<void>` returned by `registerFunction`,
which reports whether LuauAPI published the function.

## Function returns

Callbacks may return these shapes:

| C++ return | Luau result |
| --- | --- |
| `void` | No values |
| Supported scalar | One value |
| `std::optional<T>` | One value or `nil` |
| `std::tuple<...>` | Ordered multiple values |
| `std::tuple<>` | No values |
| `geode::Result<R>` | The supported `R` shape or a Luau error |

Optional tuple elements produce `nil` in their positions.
Returned `std::string` data stays owned by the invoker until LuauAPI copies it after the native function returns.
A returned `std::string_view` does not own its bytes.
Its backing storage must remain valid through this immediate post-return copy.
Protected script execution adds the normal traceback.

Pointer returns, reference returns, nested tuples, `std::optional<std::tuple<...>>`,
and custom `Result` error types are rejected. Callbacks must not throw.

## Lifetime and access

Registered leaves are ordinary mutable Luau values. Any script can call, replace, or remove them.
Native callbacks must validate their own domain rules because every script shares the runtime.

Callbacks run synchronously on the runtime thread. Long callbacks block the runtime until they return.
See [Limits and errors](limits-and-errors.md) for deadline and memory behavior.

LuauAPI stores the function pointer bytes with the Lua closure.
The registering mod and the module that owns the function pointer must stay loaded while its closure can exist.
For a wrapper, the registering mod owns the pointer.
For a directly registered dependency function, the required dependency owns it.
There is no unregister API or hot-unload support.
Runtime shutdown releases closure storage without calling the registering mod.

## Related

- [C++ API reference](api-reference.md)
- [Sharing APIs between mods](../lua/sharing-apis.md)
- [Limits and errors](limits-and-errors.md)
- [Installation](../../getting-started/installation.md)
- [Getting started](../../getting-started/overview.md)
- [LuauAPI mod guidelines](../../mod_guidelines.md)

## Source

- `include/NativeRegistration.hpp`
- `include/LuauAPI.hpp`
- `include/Export.hpp`
- `src/api.cpp`
- `src/diagnostics/BoundaryRecorder.hpp`
