#pragma once

#include <functional>
#include <std23/function_ref.h>
#include <std23/move_only_function.h>

namespace geode {
#if defined(_WIN32)
    template <class Signature>
    using Function = std::move_only_function<Signature>;
#else
    template <class Signature>
    using Function = std23::move_only_function<Signature>;
#endif

    template <class Signature>
    using FunctionRef = std23::function_ref<Signature>;
} // namespace geode
