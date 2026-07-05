# C++ standard

## Summary

LuauAPI builds as C++23.
Prefer Geode SDK utilities over hand-rolled helpers or STL equivalents when both fit.
Use `fmt` for string formatting in runtime code.

## Build requirement

CMake sets `CMAKE_CXX_STANDARD 23` and `cxx_std_23` on mod and test targets.
See `CMakeLists.txt` and `cmake/Helpers.cmake`.

You need a compiler with C++23 support.
See [Building from source](building.md).

## Geode-first utilities

Reach for [Geode Utils](https://docs.geode-sdk.org/tutorials/utils) before custom code.

| Need | Use |
| --- | --- |
| Fallible return values | `geode::Result`, `geode::Ok`, `geode::Err` |
| String split, join, trim, contains, prefix/suffix | `geode::utils::string` |
| Container find, remove, filter, map | `geode::utils::ranges` |
| File read, write, directories, atomic write | `geode::utils::file` |
| Exhaustive switch default | `geode::utils::unreachable` |
| GD type casts | `geode::cast` |
| Cocos helpers and colors | `geode::cocos` |
| CCObject lifetime | `geode::Ref`, `geode::WeakRef` |

Do not use `std::expected` for new mod APIs.
Use `geode::Result` instead.

Do not use `std::ranges` when `geode::utils::ranges` covers the case.

Codegen Geode import paths live in [Codegen](codegen/codegen.md).
Do not duplicate that table here.

## C++23 stdlib when Geode has no equivalent

These are fine when Geode does not provide a helper:

- `std::optional` monadic operations (`.and_then`, `.or_else`, `.transform`)
- `std::span`
- `concept` and `requires`
- `std::move_only_function` for move-only callbacks

## Formatting

Keep `fmt::format` and `fmt::format_to` for runtime formatting.
Do not migrate runtime code to `std::format`.

Tests may link fmt through CMake when Catch2 or host helpers need it.

## Avoid

- Raw owning pointers where RAII or Geode refs fit
- C string formatting (`sprintf`, `sscanf`) except at unavoidable C API boundaries
- `NULL` (use `nullptr`)
- Deprecated standard library APIs (`std::bind`, `std::auto_ptr`, and similar)

## Interop exceptions

Cocos, GLFW, OpenGL, and Luau C APIs sometimes require raw pointers, `new`, or `delete`.
Mark deliberate lifetime shortcuts with a short comment when the API owns the object.

## Related

- [Building from source](building.md)
- [Testing](testing.md)
- [Codegen](codegen/codegen.md)
- [Limits and errors](../reference/cpp/limits-and-errors.md)
- [Architecture](architecture.md)

## Source

- `CMakeLists.txt`
- `cmake/Helpers.cmake`
- `src/`
- `gd-imgui-cocos/`
