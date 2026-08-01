#include "core/Runtime.hpp"
#include "diagnostics/BoundaryRecorder.hpp"
#include "host/lua_test_helpers.hpp"

#include <LuauAPI.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>

namespace {
    enum class SmallEnum : std::uint8_t {
        Two = 2,
    };

    int add(int a, int b) {
        return a + b;
    }

    int addNoexcept(int a, int b) noexcept {
        return a + b;
    }

    std::int64_t roundTripInteger(std::int64_t value) {
        return value;
    }

    std::string join(std::string const& left, std::string_view right) {
        return left + std::string(right);
    }

    int optionalMix(std::optional<int> first, int second, std::optional<int> third) {
        return first.value_or(0) + second + third.value_or(0);
    }

    std::tuple<int, std::optional<std::string>, bool> tupleResult(int value) {
        return {value, std::nullopt, true};
    }

    std::tuple<> emptyTuple() {
        return {};
    }

    void noResult() {}

    geode::Result<void> checkedVoid(bool accepted) {
        if (!accepted) return geode::Err("value is rejected");
        return geode::Ok();
    }

    std::optional<int> optionalResult(bool present) {
        return present ? std::optional<int>{9} : std::nullopt;
    }

    geode::Result<int> checkedResult(int value) {
        if (value < 0) return geode::Err("negative values are rejected");
        return geode::Ok(value * 2);
    }

    bool nativeBoundaryIsActive() {
        auto const* active = luax::diag::activeBoundary();
        return active && active->kind == luax::diag::BoundaryKind::NativeFunction &&
            active->target.find("diagnostics.boundary") != std::string::npos;
    }

    int mutableReference(int& value) {
        return value;
    }

    int pointerParameter(int* value) {
        return value ? *value : 0;
    }

    std::optional<std::tuple<int>> optionalTupleReturn() {
        return std::tuple{1};
    }

    std::tuple<std::tuple<int>> nestedTupleReturn() {
        return {std::tuple{1}};
    }

    struct MemberOwner {
        int method(int value) {
            return value;
        }
    };

    auto capturingCallable = [offset = 1](int value) {
        return value + offset;
    };

    static_assert(imes::luauapi::NativeFunctionPointer<decltype(&add)>);
    static_assert(imes::luauapi::NativeFunctionPointer<decltype(&addNoexcept)>);
    static_assert(imes::luauapi::NativeFunctionPointer<decltype(&roundTripInteger)>);
    static_assert(imes::luauapi::NativeFunctionPointer<decltype(&join)>);
    static_assert(imes::luauapi::NativeFunctionPointer<decltype(&optionalMix)>);
    static_assert(imes::luauapi::NativeFunctionPointer<decltype(&tupleResult)>);
    static_assert(imes::luauapi::NativeFunctionPointer<decltype(&noResult)>);
    static_assert(imes::luauapi::NativeFunctionPointer<decltype(&checkedVoid)>);
    static_assert(imes::luauapi::NativeFunctionPointer<decltype(&checkedResult)>);
    static_assert(!imes::luauapi::NativeFunctionPointer<decltype(&mutableReference)>);
    static_assert(!imes::luauapi::NativeFunctionPointer<decltype(&pointerParameter)>);
    static_assert(!imes::luauapi::NativeFunctionPointer<decltype(&optionalTupleReturn)>);
    static_assert(!imes::luauapi::NativeFunctionPointer<decltype(&nestedTupleReturn)>);
    static_assert(!imes::luauapi::NativeFunctionPointer<decltype(&MemberOwner::method)>);
    static_assert(!imes::luauapi::NativeFunctionPointer<decltype(capturingCallable)>);
    static_assert(!imes::luauapi::NativeFunctionPointer<int (*)(wchar_t)>);
    static_assert(!imes::luauapi::NativeFunctionPointer<int (*)(char8_t)>);
    static_assert(!imes::luauapi::NativeFunctionPointer<int (*)(char16_t)>);
    static_assert(!imes::luauapi::NativeFunctionPointer<int (*)(char32_t)>);
    static_assert(!imes::luauapi::NativeFunctionPointer<wchar_t (*)()>);
    static_assert(!imes::luauapi::NativeFunctionPointer<char8_t (*)()>);
    static_assert(!imes::luauapi::NativeFunctionPointer<char16_t (*)()>);
    static_assert(!imes::luauapi::NativeFunctionPointer<char32_t (*)()>);
    static_assert(!imes::luauapi::NativeFunctionPointer<int (*)(std::optional<char16_t>)>);
    static_assert(!imes::luauapi::NativeFunctionPointer<std::optional<char32_t> (*)()>);
    static_assert(!imes::luauapi::NativeFunctionPointer<std::tuple<int, char8_t> (*)()>);
    static_assert(imes::luauapi::NativeFunctionPointer<char (*)(char)>);
    static_assert(imes::luauapi::NativeFunctionPointer<signed char (*)(signed char)>);
    static_assert(imes::luauapi::NativeFunctionPointer<unsigned char (*)(unsigned char)>);
    static_assert(imes::luauapi::NativeFunctionPointer<SmallEnum (*)(SmallEnum)>);

    using ConstCharArray = char const[4];
    using VolatileCharArray = char volatile[4];

    static_assert(imes::luauapi::NativeValue<std::string>);
    static_assert(imes::luauapi::NativeValue<std::string const&>);
    static_assert(imes::luauapi::NativeValue<std::string_view>);
    static_assert(imes::luauapi::NativeValue<std::string_view const&>);
    static_assert(imes::luauapi::NativeValue<char*>);
    static_assert(imes::luauapi::NativeValue<char const*>);
    static_assert(imes::luauapi::NativeValue<ConstCharArray&>);
    static_assert(imes::luauapi::NativeValue<char>);
    static_assert(imes::luauapi::NativeValue<signed char>);
    static_assert(imes::luauapi::NativeValue<unsigned char>);
    static_assert(imes::luauapi::NativeValue<SmallEnum>);
    static_assert(!imes::luauapi::NativeValue<std::string volatile&>);
    static_assert(!imes::luauapi::NativeValue<std::string_view volatile&>);
    static_assert(!imes::luauapi::NativeValue<char volatile*>);
    static_assert(!imes::luauapi::NativeValue<VolatileCharArray&>);
    static_assert(!imes::luauapi::NativeValue<wchar_t>);
    static_assert(!imes::luauapi::NativeValue<char8_t>);
    static_assert(!imes::luauapi::NativeValue<char16_t>);
    static_assert(!imes::luauapi::NativeValue<char32_t>);
    static_assert(!imes::luauapi::NativeValue<std::optional<char16_t>>);

    geode::Mod* makeProvider(
        luauapi_test::ScopedTempDir const& temp, std::string id = "provider.mod-with-dash"
    ) {
        auto* provider = geode::Mod::create(temp.path, std::move(id));
        geode::Mod::setFallbackMod(provider);
        return provider;
    }
} // namespace

TEST_CASE("native registration publishes exact provider namespace and nested paths") {
    luauapi_test::ModRuntimeGuard guard;
    luauapi_test::ScopedTempDir temp{"luauapi_native_registration_"};
    makeProvider(temp);

    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    int const top = lua_gettop(L);
    REQUIRE(imes::luauapi::registerValue("math.defaultDivisor", 7).isOk());
    REQUIRE(imes::luauapi::registerValue("metadata.name", std::string("api\0v1", 6)).isOk());
    REQUIRE(imes::luauapi::registerValue("metadata.enum", SmallEnum::Two).isOk());
    REQUIRE(imes::luauapi::registerFunction("math.add", &add).isOk());
    REQUIRE(lua_gettop(L) == top);

    REQUIRE(
        luauapi_test::runScriptReturnsBool(
            L,
            R"(
            local api = _G["provider.mod-with-dash"]
            return api.math.defaultDivisor == 7
                and api.metadata.enum == 2
                and #api.metadata.name == 6
                and string.byte(api.metadata.name, 4) == 0
                and api.math.add(2, 3) == 5
                and _G.provider == nil
        )"
        )
    );
}

TEST_CASE("native functions marshal optional refs tuples and Results") {
    luauapi_test::ModRuntimeGuard guard;
    luauapi_test::ScopedTempDir temp{"luauapi_native_marshalling_"};
    makeProvider(temp, "marshal.mod");

    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    REQUIRE(imes::luauapi::registerFunction("text.join", &join).isOk());
    REQUIRE(imes::luauapi::registerFunction("optional.mix", &optionalMix).isOk());
    REQUIRE(imes::luauapi::registerFunction("returns.tuple", &tupleResult).isOk());
    REQUIRE(imes::luauapi::registerFunction("returns.empty", &emptyTuple).isOk());
    REQUIRE(imes::luauapi::registerFunction("returns.void", &noResult).isOk());
    REQUIRE(imes::luauapi::registerFunction("returns.checkedVoid", &checkedVoid).isOk());
    REQUIRE(imes::luauapi::registerFunction("returns.optional", &optionalResult).isOk());
    REQUIRE(imes::luauapi::registerFunction("returns.checked", &checkedResult).isOk());

    REQUIRE(
        luauapi_test::runScriptReturnsBool(
            L,
            R"(
            local api = _G["marshal.mod"]
            local a, b, c = api.returns.tuple(4)
            local emptyCount = select("#", api.returns.empty())
            local voidCount = select("#", api.returns.void())
            local checkedVoidCount = select("#", api.returns.checkedVoid(true))
            return api.text.join("ab", "cd") == "abcd"
                and api.optional.mix(nil, 4) == 4
                and api.optional.mix(2, 4, 3) == 9
                and a == 4 and b == nil and c == true
                and emptyCount == 0
                and voidCount == 0
                and checkedVoidCount == 0
                and api.returns.optional(true) == 9
                and api.returns.optional(false) == nil
                and api.returns.checked(3) == 6
        )"
        )
    );

    auto error = luauapi_test::runScriptReturnsString(
        L,
        R"(
            local ok, err = pcall(function()
                _G["marshal.mod"].returns.checked(-1)
            end)
            assert(not ok)
            return tostring(err)
        )"
    );
    REQUIRE(error.has_value());
    REQUIRE(error->find(R"(_G["marshal.mod"].returns.checked)") != std::string::npos);
    REQUIRE(error->find("negative values are rejected") != std::string::npos);

    auto protectedError = imes::luauapi::runScript(
        temp.path,
        R"(
            local function invoke()
                _G["marshal.mod"].returns.checked(-1)
            end
            invoke()
        )",
        "native-error.luau"
    );
    REQUIRE(protectedError.isErr());
    auto const traceback = imes::luauapi::lastError();
    REQUIRE(traceback.find(R"(_G["marshal.mod"].returns.checked)") != std::string::npos);
    REQUIRE(traceback.find("native-error.luau") != std::string::npos);
}

TEST_CASE("native functions reject wrong arity types and unsafe numbers") {
    luauapi_test::ModRuntimeGuard guard;
    luauapi_test::ScopedTempDir temp{"luauapi_native_validation_"};
    makeProvider(temp, "validation.mod");

    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    REQUIRE(imes::luauapi::registerFunction("add", &add).isOk());
    REQUIRE(imes::luauapi::registerFunction("optional", &optionalMix).isOk());
    REQUIRE(imes::luauapi::registerFunction("roundTrip", &roundTripInteger).isOk());

    REQUIRE(
        luauapi_test::runScriptReturnsBool(
            L,
            R"(
            local api = _G["validation.mod"]
            local okFew = pcall(api.add, 1)
            local okMany = pcall(api.add, 1, 2, 3)
            local okFraction = pcall(api.add, 1.5, 2)
            local okType = pcall(api.add, "1", 2)
            local okMissingRequired = pcall(api.optional)
            return not okFew and not okMany and not okFraction and not okType
                and not okMissingRequired
        )"
        )
    );

    REQUIRE(imes::luauapi::registerValue("max", (std::numeric_limits<std::int64_t>::max)()).isOk());
    REQUIRE(imes::luauapi::registerValue("min", (std::numeric_limits<std::int64_t>::min)()).isOk());
    REQUIRE(
        luauapi_test::runScriptReturnsBool(
            L,
            R"(return tostring(_G["validation.mod"].max) == "9223372036854775807"
            and tostring(_G["validation.mod"].roundTrip(_G["validation.mod"].min))
                == "-9223372036854775808")"
        )
    );
    REQUIRE(
        imes::luauapi::registerValue("unsignedTooLarge", (std::numeric_limits<std::uint64_t>::max)())
            .isErr()
    );
    REQUIRE(imes::luauapi::registerValue("nan", std::numeric_limits<double>::quiet_NaN()).isErr());
    REQUIRE(imes::luauapi::registerValue("infinity", std::numeric_limits<double>::infinity()).isErr());
}

TEST_CASE("native registration conflicts are atomic and preserve the Lua stack") {
    luauapi_test::ModRuntimeGuard guard;
    luauapi_test::ScopedTempDir temp{"luauapi_native_conflicts_"};
    makeProvider(temp, "conflict.mod");

    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    REQUIRE(imes::luauapi::registerValue("stable.value", 11).isOk());
    REQUIRE(luauapi_test::runScriptVoid(L, R"(_G["conflict.mod"].blocked = 12)"));

    int const top = lua_gettop(L);
    auto duplicate = imes::luauapi::registerValue("stable.value", 99);
    auto intermediate = imes::luauapi::registerValue("blocked.child", 5);
    auto invalidLeading = imes::luauapi::registerValue(".bad", 1);
    auto invalidTrailing = imes::luauapi::registerValue("bad.", 1);
    auto invalidEmpty = imes::luauapi::registerValue("bad..path", 1);

    REQUIRE(duplicate.isErr());
    REQUIRE(intermediate.isErr());
    REQUIRE(invalidLeading.isErr());
    REQUIRE(invalidTrailing.isErr());
    REQUIRE(invalidEmpty.isErr());
    REQUIRE(lua_gettop(L) == top);
    REQUIRE(
        luauapi_test::runScriptReturnsBool(
            L,
            R"(
            local api = _G["conflict.mod"]
            return api.stable.value == 11 and api.blocked == 12 and api.bad == nil
        )"
        )
    );

    auto* other = geode::Mod::create(temp.path, "occupied.mod");
    geode::Mod::setFallbackMod(other);
    REQUIRE(luauapi_test::runScriptVoid(L, R"(_G["occupied.mod"] = 42)"));
    REQUIRE(imes::luauapi::registerValue("value", 1).isErr());
    REQUIRE(luauapi_test::runScriptReturnsBool(L, R"(return _G["occupied.mod"] == 42)"));
}

TEST_CASE("native closures survive leaf overwrite while retained") {
    luauapi_test::ModRuntimeGuard guard;
    luauapi_test::ScopedTempDir temp{"luauapi_native_lifetime_"};
    makeProvider(temp, "lifetime.mod");

    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    REQUIRE(imes::luauapi::registerFunction("add", &add).isOk());
    REQUIRE(
        luauapi_test::runScriptVoid(
            L,
            R"(
            _G.retainedNativeFunction = _G["lifetime.mod"].add
            _G["lifetime.mod"].add = nil
        )"
        )
    );
    luauapi_test::collectGarbage(L);
    REQUIRE(luauapi_test::runScriptReturnsBool(L, R"(return _G.retainedNativeFunction(8, 9) == 17)"));
}

TEST_CASE("native callbacks record a dedicated diagnostic boundary") {
    luauapi_test::ModRuntimeGuard guard;
    luauapi_test::ScopedTempDir temp{"luauapi_native_boundary_"};
    makeProvider(temp, "diagnostics.mod");

    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);
    auto* L = runtime->state();
    REQUIRE(L != nullptr);

    luax::diag::resetForTests();
    luax::diag::setRecordingEnabled(true);
    REQUIRE(imes::luauapi::registerFunction("diagnostics.boundary", &nativeBoundaryIsActive).isOk());
    REQUIRE(
        luauapi_test::runScriptReturnsBool(L, R"(return _G["diagnostics.mod"].diagnostics.boundary())")
    );
    REQUIRE(luax::diag::activeBoundary() == nullptr);
}

TEST_CASE("native registration rejects null empty not-ready off-thread and shutdown calls") {
    luauapi_test::ModRuntimeGuard guard;
    luauapi_test::ScopedTempDir temp{"luauapi_native_state_"};
    makeProvider(temp, "state.mod");

    REQUIRE(imes::luauapi::registerValue("beforeReady", 1).isErr());

    auto* runtime = luax::Runtime::getOrCreate();
    REQUIRE(runtime != nullptr);

    std::optional<int> empty;
    char const* nullString = nullptr;
    int (*nullFunction)(int, int) = nullptr;
    REQUIRE(imes::luauapi::registerValue("empty", empty).isErr());
    REQUIRE(imes::luauapi::registerValue("null", nullString).isErr());
    REQUIRE(imes::luauapi::registerFunction("nullFunction", nullFunction).isErr());

    std::optional<geode::Result<void>> offThread;
    std::thread worker([&] {
        offThread = imes::luauapi::registerValue("offThread", 1);
    });
    worker.join();
    REQUIRE(offThread.has_value());
    REQUIRE(offThread->isErr());
    REQUIRE(offThread->unwrapErr().find("main thread") != std::string::npos);

    luax::Runtime::shutdown();
    auto shutdown = imes::luauapi::registerValue("shutdown", 1);
    REQUIRE(shutdown.isErr());
    REQUIRE(shutdown.unwrapErr() == "luau runtime shutting down");
}
