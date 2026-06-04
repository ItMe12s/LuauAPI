#pragma once

#include "lua/Config.hpp"

#include <cstddef>

namespace luax {
    inline bool allocatorCanReallocate(
        std::size_t usage, std::size_t limit, std::size_t osize, std::size_t nsize
    ) {
        if (nsize <= osize) {
            return true;
        }
        auto delta = nsize - osize;
        return usage <= limit && delta <= limit - usage;
    }

    inline std::size_t allocatorUsageAfterReallocate(
        std::size_t usage, std::size_t osize, std::size_t nsize
    ) {
        if (osize <= usage) {
            return usage - osize + nsize;
        }
        return nsize;
    }

    inline std::size_t allocatorUsageAfterFree(std::size_t usage, std::size_t osize) {
        return osize <= usage ? usage - osize : 0;
    }
} // namespace luax
