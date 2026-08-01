# Native C++ registration

## Summary

Provider mods can publish typed C++ functions and values into LuauAPI's shared global table.
LuauAPI places each API under the calling mod's full id.
Registration is direct and does not expose the Luau stack to provider mods.

## Setup

Declare LuauAPI as a required Geode dependency.
Include `LuauAPI.hpp` from the dependency's exported include directory.
See [Installation](../../getting-started/installation.md) for dependency setup.

Registration must run on the main thread after the Luau runtime is ready.
It returns `Err` before readiness, during shutdown, or off the main thread.
It does not create the runtime or queue work for later.

```cpp
geode::Result<int> add(int a, int b) {
    return geode::Ok(a + b);
}

$on_mod(Loaded) {
    using namespace imes::luauapi;

    auto functionResult = registerFunction("math.add", &add);
    if (functionResult.isErr()) {
        geode::log::error("Could not register math.add: {}", functionResult.unwrapErr());
        return;
    }

    auto valueResult = registerValue("metadata.version", 1);
    if (valueResult.isErr()) {
        geode::log::error("Could not register metadata.version: {}", valueResult.unwrapErr());
    }
}
```

## Published layout

The caller-local `geode::Mod::get()` supplies the provider id.
LuauAPI does not resolve the provider inside its own binary.

For a provider named `provider.mod`, the example publishes:

```luau
_G["provider.mod"].math.add(2, 3)
_G["provider.mod"].metadata.version
```

The full provider id is one `_G` key, including dots and dashes.
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
`std::function`, nested optionals, wide unsupported numerics, and C-string parameters are rejected.

Arity is strict. The maximum is the declared parameter count. The minimum ends at the last required parameter.
Omitted arguments work only when every remaining parameter is optional.
A non-trailing optional needs an explicit value or `nil` when a required parameter follows it.
Colon calls add a `self` argument and fail unless the signature accepts that argument.

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
String results are copied into Luau before the callback returns.
A callback `Result::Err` raises an error that includes the qualified registered target.
Protected script execution adds the normal traceback.

Pointer returns, reference returns, nested tuples, `std::optional<std::tuple<...>>`,
and custom `Result` error types are rejected. Callbacks must not throw.
Use `geode::Result` for recoverable failures.

## Lifetime and access

Registered leaves are ordinary mutable Luau values. Any script can call, replace, or remove them.
Native callbacks must validate their own domain rules because every script shares the runtime.

Callbacks run synchronously on the runtime thread. Long callbacks block the runtime until they return.
See [Limits and errors](limits-and-errors.md) for deadline and memory behavior.

LuauAPI stores the function pointer bytes with the Lua closure.
The provider mod must stay loaded while a registered closure can exist.
There is no unregister API or hot-unload support.
Runtime shutdown releases closure storage without calling the provider.

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
