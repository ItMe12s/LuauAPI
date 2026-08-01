#pragma once

#include "Export.hpp"

#include <Geode/Geode.hpp>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace imes::luauapi {
    namespace detail {
        enum class NativeValueKind : std::uint8_t {
            Nil,
            Boolean,
            Integer,
            Number,
            String,
            Unsupported,
        };

        struct NativeValue {
            NativeValueKind kind = NativeValueKind::Nil;
            std::uint8_t booleanValue = 0;
            std::int64_t integerValue = 0;
            double numberValue = 0.0;
            char const* stringData = nullptr;
            std::uint64_t stringSize = 0;
        };

        struct NativeCall;

        struct NativeCallOps {
            std::uint32_t (*argumentCount)(void* state) noexcept;
            NativeValueKind (*argumentKind)(void* state, std::uint32_t index) noexcept;
            std::uint8_t (*readBoolean)(void* state, std::uint32_t index, std::uint8_t* out) noexcept;
            std::uint8_t (*readInteger)(void* state, std::uint32_t index, std::int64_t* out) noexcept;
            std::uint8_t (*readNumber)(void* state, std::uint32_t index, double* out) noexcept;
            std::uint8_t (*readString)(
                void* state, std::uint32_t index, char const** data, std::uint64_t* size
            ) noexcept;
            void (*pushNil)(void* state);
            void (*pushBoolean)(void* state, std::uint8_t value);
            void (*pushInteger)(void* state, std::int64_t value);
            void (*pushNumber)(void* state, double value);
            void (*pushString)(void* state, char const* data, std::uint64_t size);
            void (*setError)(void* state, char const* data, std::uint64_t size);
        };

        struct NativeCall {
            void* state = nullptr;
            NativeCallOps const* ops = nullptr;
        };

        using NativeInvoker =
            std::int32_t (*)(void const* functionBytes, std::uint64_t functionSize, NativeCall* call);

        static_assert(std::is_trivially_copyable_v<NativeValue>);
        static_assert(std::is_trivially_copyable_v<NativeCallOps>);
        static_assert(std::is_trivially_copyable_v<NativeCall>);

        LUAUAPI_DLL geode::Result<void> registerNativeFunction(
            geode::Mod* provider, char const* pathData, std::uint64_t pathSize,
            NativeInvoker invoker, void const* functionBytes, std::uint64_t functionSize
        );

        LUAUAPI_DLL geode::Result<void> registerNativeValue(
            geode::Mod* provider, char const* pathData, std::uint64_t pathSize, NativeValue const* value
        );

        template <class T>
        using Bare = std::remove_cv_t<std::remove_reference_t<T>>;

        template <class T>
        struct OptionalTraits {
            static constexpr bool value = false;
        };

        template <class T>
        struct OptionalTraits<std::optional<T>> {
            static constexpr bool value = true;
            using Value = T;
        };

        template <class T>
        inline constexpr bool isOptional = OptionalTraits<Bare<T>>::value;

        template <class T>
        struct TupleTraits {
            static constexpr bool value = false;
        };

        template <class... T>
        struct TupleTraits<std::tuple<T...>> {
            static constexpr bool value = true;
        };

        template <class T>
        inline constexpr bool isTuple = TupleTraits<Bare<T>>::value;

        template <class T>
        struct ResultTraits {
            static constexpr bool value = false;
        };

        template <class T>
        struct ResultTraits<geode::Result<T>> {
            static constexpr bool value = true;
            using Value = T;
        };

        template <class T>
        inline constexpr bool isResult = ResultTraits<Bare<T>>::value;

        template <class T>
        inline constexpr bool isString = !std::is_volatile_v<std::remove_reference_t<T>> &&
            (std::same_as<Bare<T>, std::string> || std::same_as<Bare<T>, std::string_view>);

        template <class T>
        inline constexpr bool isInteger = [] {
            if constexpr (std::integral<Bare<T>>) {
                return !std::same_as<Bare<T>, bool> && !std::same_as<Bare<T>, wchar_t> &&
                    !std::same_as<Bare<T>, char8_t> && !std::same_as<Bare<T>, char16_t> &&
                    !std::same_as<Bare<T>, char32_t> && sizeof(Bare<T>) <= 8;
            }
            else if constexpr (std::is_enum_v<Bare<T>>) {
                return sizeof(std::underlying_type_t<Bare<T>>) <= 8;
            }
            else {
                return false;
            }
        }();

        template <class T>
        inline constexpr bool isNumber =
            std::same_as<Bare<T>, float> || std::same_as<Bare<T>, double>;

        template <class T>
        inline constexpr bool isScalar =
            std::same_as<Bare<T>, bool> || isInteger<T> || isNumber<T> || isString<T>;
        constexpr std::int64_t kLuauMaxSafeInteger = 9007199254740991LL;
        constexpr std::int64_t kLuauMinSafeInteger = -kLuauMaxSafeInteger;

        template <class T>
        bool encodeLuauInteger(T value, std::int64_t& out) {
            using Value = Bare<T>;
            if constexpr (std::is_enum_v<Value>) {
                return encodeLuauInteger(static_cast<std::underlying_type_t<Value>>(value), out);
            }
            else {
                if (!std::in_range<std::int64_t>(value)) return false;
                out = static_cast<std::int64_t>(value);
                return true;
            }
        }

        template <class T>
        inline constexpr bool isOptionalScalar = [] {
            if constexpr (!isOptional<T>) return false;
            else return isScalar<typename OptionalTraits<Bare<T>>::Value>;
        }();

        template <class T>
        inline constexpr bool isParameter = [] {
            if constexpr (std::is_rvalue_reference_v<T>) {
                return false;
            }
            else if constexpr (std::is_lvalue_reference_v<T> && !std::is_const_v<std::remove_reference_t<T>>) {
                return false;
            }
            else {
                return isScalar<T> || isOptionalScalar<T>;
            }
        }();

        template <class T>
        inline constexpr bool isTupleReturn = [] {
            if constexpr (!isTuple<T>) {
                return false;
            }
            else {
                return []<class... V>(std::tuple<V...>*) {
                    return ((!std::is_reference_v<V> && (isScalar<V> || isOptionalScalar<V>)) && ...);
                }(static_cast<Bare<T>*>(nullptr));
            }
        }();

        template <class T>
        inline constexpr bool isReturnPayload = std::same_as<Bare<T>, void> ||
            (!std::is_reference_v<T> && (isScalar<T> || isOptionalScalar<T> || isTupleReturn<T>));

        template <class T>
        inline constexpr bool isNativeReturn = [] {
            if constexpr (isResult<T>) {
                return isReturnPayload<typename ResultTraits<Bare<T>>::Value>;
            }
            else {
                return isReturnPayload<T>;
            }
        }();

        template <class T>
        struct FunctionTraits {
            static constexpr bool value = false;
        };

        template <class R, class... Args>
        struct FunctionTraits<R (*)(Args...)> {
            static constexpr bool value = isNativeReturn<R> && (isParameter<Args> && ...);
            using Return = R;
            using Arguments = std::tuple<Args...>;
        };

        template <class R, class... Args>
        struct FunctionTraits<R (*)(Args...) noexcept> : FunctionTraits<R (*)(Args...)> {};

        template <class T>
        inline constexpr bool isNativeFunctionPointer = FunctionTraits<Bare<T>>::value;

        inline void setCallError(NativeCall& call, std::string_view message) {
            call.ops->setError(call.state, message.data(), static_cast<std::uint64_t>(message.size()));
        }

        template <class T>
        constexpr std::string_view nativeTypeName() {
            if constexpr (std::same_as<Bare<T>, bool>) return "boolean";
            else if constexpr (isInteger<T>) return "integer";
            else if constexpr (isNumber<T>) return "finite number";
            else if constexpr (isString<T>) return "string";
            else return "value";
        }

        template <class T>
        bool readScalar(NativeCall& call, std::uint32_t index, T& out) {
            using Value = Bare<T>;

            bool ok = false;
            if constexpr (std::same_as<Value, bool>) {
                std::uint8_t value = 0;
                ok = call.ops->readBoolean(call.state, index, &value);
                if (ok) out = value != 0;
            }
            else if constexpr (isInteger<Value>) {
                std::int64_t value = 0;
                ok = call.ops->readInteger(call.state, index, &value);
                if (ok) {
                    if constexpr (std::is_enum_v<Value>) {
                        using Underlying = std::underlying_type_t<Value>;
                        if (!std::in_range<Underlying>(value)) ok = false;
                        else out = static_cast<Value>(static_cast<Underlying>(value));
                    }
                    else {
                        if (!std::in_range<Value>(value)) ok = false;
                        else out = static_cast<Value>(value);
                    }
                }
            }
            else if constexpr (isNumber<Value>) {
                double value = 0.0;
                ok = call.ops->readNumber(call.state, index, &value) && std::isfinite(value);
                if (ok) {
                    if constexpr (std::same_as<Value, float>) {
                        if (value < -(std::numeric_limits<float>::max)() ||
                            value > (std::numeric_limits<float>::max)()) {
                            ok = false;
                        }
                        else {
                            out = static_cast<float>(value);
                        }
                    }
                    else {
                        out = value;
                    }
                }
            }
            else if constexpr (std::same_as<Value, std::string>) {
                char const* data = nullptr;
                std::uint64_t size = 0;
                ok = call.ops->readString(call.state, index, &data, &size);
                if (ok && size <= (std::numeric_limits<std::size_t>::max)()) {
                    out.assign(data, static_cast<std::size_t>(size));
                }
                else {
                    ok = false;
                }
            }
            else if constexpr (std::same_as<Value, std::string_view>) {
                char const* data = nullptr;
                std::uint64_t size = 0;
                ok = call.ops->readString(call.state, index, &data, &size);
                if (ok && size <= (std::numeric_limits<std::size_t>::max)()) {
                    out = std::string_view(data, static_cast<std::size_t>(size));
                }
                else {
                    ok = false;
                }
            }

            if (!ok) {
                setCallError(
                    call,
                    "argument " + std::to_string(index + 1) + " must be " +
                        std::string(nativeTypeName<Value>())
                );
            }
            return ok;
        }

        template <class Arg>
        bool readArgument(NativeCall& call, std::uint32_t index, Bare<Arg>& out) {
            if constexpr (isOptional<Arg>) {
                if (call.ops->argumentKind(call.state, index) == NativeValueKind::Nil) {
                    out.reset();
                    return true;
                }
                typename OptionalTraits<Bare<Arg>>::Value value{};
                if (!readScalar(call, index, value)) return false;
                out = std::move(value);
                return true;
            }
            else {
                return readScalar(call, index, out);
            }
        }

        template <class Arg, class Storage>
        decltype(auto) argumentForCall(Storage& value) {
            if constexpr (std::is_lvalue_reference_v<Arg>) {
                return static_cast<Arg>(value);
            }
            else {
                return static_cast<Arg>(std::move(value));
            }
        }

        template <class T>
        bool pushScalar(NativeCall& call, T const& value) {
            using Value = Bare<T>;

            if constexpr (std::same_as<Value, bool>) {
                call.ops->pushBoolean(call.state, value ? 1 : 0);
            }
            else if constexpr (isInteger<Value>) {
                std::int64_t encoded = 0;
                if (!encodeLuauInteger(value, encoded)) {
                    setCallError(call, "return integer is outside signed 64-bit range");
                    return false;
                }
                call.ops->pushInteger(call.state, encoded);
            }
            else if constexpr (isNumber<Value>) {
                double encoded = static_cast<double>(value);
                if (!std::isfinite(encoded)) {
                    setCallError(call, "return number must be finite");
                    return false;
                }
                call.ops->pushNumber(call.state, encoded);
            }
            else if constexpr (isString<Value>) {
                std::string_view encoded(value);
                call.ops->pushString(
                    call.state, encoded.data(), static_cast<std::uint64_t>(encoded.size())
                );
            }
            return true;
        }

        template <class T>
        bool pushReturnElement(NativeCall& call, T const& value) {
            if constexpr (isOptional<T>) {
                if (!value) {
                    call.ops->pushNil(call.state);
                    return true;
                }
                return pushScalar(call, *value);
            }
            else {
                return pushScalar(call, value);
            }
        }

        template <class T>
        int pushReturnPayload(NativeCall& call, T const& value) {
            if constexpr (isTuple<T>) {
                bool ok = std::apply(
                    [&](auto const&... element) {
                        return (pushReturnElement(call, element) && ...);
                    },
                    value
                );
                if (!ok) return -1;
                return static_cast<int>(std::tuple_size_v<Bare<T>>);
            }
            else {
                return pushReturnElement(call, value) ? 1 : -1;
            }
        }

        template <class T>
        int pushNativeReturn(NativeCall& call, T const& value) {
            if constexpr (isResult<T>) {
                if (value.isErr()) {
                    auto const& error = value.unwrapErr();
                    setCallError(call, std::string_view(error));
                    return -1;
                }
                using ResultValue = typename ResultTraits<Bare<T>>::Value;
                if constexpr (std::same_as<ResultValue, void>) {
                    return 0;
                }
                else {
                    return pushReturnPayload(call, value.unwrap());
                }
            }
            else {
                return pushReturnPayload(call, value);
            }
        }

        template <class... Args>
        consteval std::uint32_t minimumArgumentCount() {
            std::uint32_t minimum = 0;
            std::uint32_t index = 0;
            if constexpr (sizeof...(Args) > 0) {
                ((++index, isOptional<Args> ? void() : void(minimum = index)), ...);
            }
            return minimum;
        }

        template <class Fn, class R, class... Args, std::size_t... I>
        int invokeNativeFunctionImpl(
            Fn function, NativeCall& call, std::tuple<Bare<Args>...>& arguments,
            std::index_sequence<I...>
        ) {
            if (!(readArgument<Args>(call, static_cast<std::uint32_t>(I), std::get<I>(arguments)) &&
                  ...)) {
                return -1;
            }

            if constexpr (std::same_as<R, void>) {
                std::invoke(function, argumentForCall<Args>(std::get<I>(arguments))...);
                return 0;
            }
            else {
                auto result = std::invoke(function, argumentForCall<Args>(std::get<I>(arguments))...);
                return pushNativeReturn(call, result);
            }
        }

        template <class Fn, class R, class... Args>
        int invokeNativeFunction(
            void const* functionBytes, std::uint64_t functionSize, NativeCall& call,
            std::tuple<Args...>*
        ) {
            if (!functionBytes || functionSize != sizeof(Fn)) {
                setCallError(call, "native function descriptor is invalid");
                return -1;
            }

            constexpr auto maximum = static_cast<std::uint32_t>(sizeof...(Args));
            constexpr auto minimum = minimumArgumentCount<Args...>();
            auto const count = call.ops->argumentCount(call.state);
            if (count < minimum || count > maximum) {
                setCallError(
                    call,
                    "expected " + std::to_string(minimum) +
                        (minimum == maximum ? "" : " to " + std::to_string(maximum)) +
                        " arguments, got " + std::to_string(count)
                );
                return -1;
            }

            Fn function{};
            std::memcpy(&function, functionBytes, sizeof(function));
            if (!function) {
                setCallError(call, "native function pointer is null");
                return -1;
            }

            std::tuple<Bare<Args>...> arguments;
            return invokeNativeFunctionImpl<Fn, R, Args...>(
                function, call, arguments, std::index_sequence_for<Args...>{}
            );
        }

        template <class Fn>
        std::int32_t nativeFunctionInvoker(
            void const* functionBytes, std::uint64_t functionSize, NativeCall* call
        ) {
            if (!call) return -1;
            using Traits = FunctionTraits<Fn>;
            return invokeNativeFunction<Fn, typename Traits::Return>(
                functionBytes, functionSize, *call, static_cast<typename Traits::Arguments*>(nullptr)
            );
        }

        template <class T>
        inline constexpr bool isCharArray = std::is_array_v<std::remove_reference_t<T>> &&
            !std::is_volatile_v<std::remove_extent_t<std::remove_reference_t<T>>> &&
            std::same_as<std::remove_cv_t<std::remove_extent_t<std::remove_reference_t<T>>>, char>;

        template <class T>
        inline constexpr bool isCharPointer =
            std::is_pointer_v<Bare<T>> && !std::is_volatile_v<std::remove_pointer_t<Bare<T>>> &&
            std::same_as<std::remove_cv_t<std::remove_pointer_t<Bare<T>>>, char>;

        template <class T>
        inline constexpr bool isRegisteredValue =
            isScalar<T> || isOptionalScalar<T> || isCharArray<T> || isCharPointer<T>;

        template <class T>
        bool encodeNativeValue(T const& value, NativeValue& out, std::string& error) {
            using Value = Bare<T>;
            if constexpr (std::same_as<Value, bool>) {
                out.kind = NativeValueKind::Boolean;
                out.booleanValue = value ? 1 : 0;
            }
            else if constexpr (isInteger<Value>) {
                if (!encodeLuauInteger(value, out.integerValue)) {
                    error = "registered integer is outside signed 64-bit range";
                    return false;
                }
                out.kind = NativeValueKind::Integer;
            }
            else if constexpr (isNumber<Value>) {
                double encoded = static_cast<double>(value);
                if (!std::isfinite(encoded)) {
                    error = "registered number must be finite";
                    return false;
                }
                out.kind = NativeValueKind::Number;
                out.numberValue = encoded;
            }
            else if constexpr (isString<Value>) {
                std::string_view encoded(value);
                out.kind = NativeValueKind::String;
                out.stringData = encoded.data();
                out.stringSize = static_cast<std::uint64_t>(encoded.size());
            }
            else if constexpr (isCharArray<T>) {
                constexpr std::size_t size = std::extent_v<std::remove_reference_t<T>>;
                out.kind = NativeValueKind::String;
                out.stringData = value;
                out.stringSize =
                    static_cast<std::uint64_t>(size > 0 && value[size - 1] == '\0' ? size - 1 : size);
            }
            else if constexpr (isCharPointer<T>) {
                if (!value) {
                    error = "registered C string is null";
                    return false;
                }
                out.kind = NativeValueKind::String;
                out.stringData = value;
                out.stringSize = static_cast<std::uint64_t>(std::char_traits<char>::length(value));
            }
            return true;
        }
    } // namespace detail

    template <class T>
    concept NativeFunctionPointer = detail::isNativeFunctionPointer<T>;

    template <class T>
    concept NativeValue = detail::isRegisteredValue<T>;

    template <NativeFunctionPointer Fn>
    geode::Result<void> registerFunction(std::string_view path, Fn function) {
        auto* provider = geode::Mod::get();
        if (!provider) return geode::Err("native registration has no provider mod");
        if (!function) return geode::Err("native function pointer is null");
        return detail::registerNativeFunction(
            provider,
            path.data(),
            static_cast<std::uint64_t>(path.size()),
            &detail::nativeFunctionInvoker<Fn>,
            &function,
            sizeof(function)
        );
    }

    template <NativeValue T>
    geode::Result<void> registerValue(std::string_view path, T&& value) {
        auto* provider = geode::Mod::get();
        if (!provider) return geode::Err("native registration has no provider mod");

        detail::NativeValue encoded;
        std::string error;
        if constexpr (detail::isOptional<T>) {
            if (!value) return geode::Err("registered optional value is empty");
            if (!detail::encodeNativeValue(*value, encoded, error)) return geode::Err(error);
        }
        else {
            if (!detail::encodeNativeValue(value, encoded, error)) return geode::Err(error);
        }
        return detail::registerNativeValue(
            provider, path.data(), static_cast<std::uint64_t>(path.size()), &encoded
        );
    }
} // namespace imes::luauapi
