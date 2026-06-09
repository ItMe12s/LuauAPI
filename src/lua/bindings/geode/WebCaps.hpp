#pragma once

#include "lua/Config.hpp"
#include "lua/bindings/framework/Stack.hpp"

#include <cstddef>
#include <lua.h>
#include <matjson.hpp>
#include <string>

namespace luax {
    inline constexpr char kWebResponseSizeExceededMsg[] = "response exceeds maximum size";
    inline constexpr char kWebRequestBodyExceededMsg[] = "request body exceeds maximum size";

    inline bool responseDataWithinLimit(std::size_t size) {
        return size <= kMaxWebResponseBytes;
    }

    inline bool requestBodyWithinLimit(std::size_t size) {
        return size <= kMaxWebRequestBytes;
    }

    inline bool requestJsonBodyWithinLimit(matjson::Value const& value) {
        return requestBodyWithinLimit(value.dump(matjson::NO_INDENTATION).size());
    }

    inline int pushResponseSizeExceeded(lua_State* L) {
        return pushNilErr(L, kWebResponseSizeExceededMsg);
    }

    inline int pushRequestBodyExceeded(lua_State* L) {
        return pushNilErr(L, kWebRequestBodyExceededMsg);
    }
} // namespace luax
