#pragma once

#include <cstddef>
#include <string>

namespace luax {
    inline std::size_t bytecodeEntryBytes(std::string const& bytecode) {
        return bytecode.size();
    }

    inline std::size_t bytecodeCacheUsageAfterInsert(std::size_t usage, std::size_t entryBytes) {
        return usage + entryBytes;
    }

    inline std::size_t bytecodeCacheUsageAfterRemove(std::size_t usage, std::size_t entryBytes) {
        return entryBytes <= usage ? usage - entryBytes : 0;
    }

    inline bool bytecodeCacheNeedsEviction(
        std::size_t usage, std::size_t limit, std::size_t incomingBytes, std::size_t entryCount,
        std::size_t maxEntries
    ) {
        if (entryCount >= maxEntries) {
            return entryCount > 0;
        }
        if (incomingBytes > limit) {
            return false;
        }
        return usage + incomingBytes > limit && entryCount > 0;
    }

    inline bool memoryBudgetAllows(std::size_t usage, std::size_t limit, std::size_t additional) {
        return additional <= limit && usage <= limit - additional;
    }

    inline bool bytecodeCacheInsertNeedsEviction(
        std::size_t cacheUsage, std::size_t cacheLimit, std::size_t incomingBytes,
        std::size_t entryCount, std::size_t maxEntries, std::size_t memoryUsage, std::size_t memoryLimit
    ) {
        if (entryCount == 0) {
            return false;
        }
        if (bytecodeCacheNeedsEviction(cacheUsage, cacheLimit, incomingBytes, entryCount, maxEntries)) {
            return true;
        }
        return !memoryBudgetAllows(memoryUsage, memoryLimit, incomingBytes);
    }

    inline bool compileTimeWithinBudget(long long compileMs, int budgetMs) {
        return budgetMs <= 0 || compileMs <= budgetMs;
    }
} // namespace luax
