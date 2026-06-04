#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace luax {
    template <typename T>
    concept HasSlotId = requires(T& value) {
        { value.id } -> std::same_as<std::uint64_t&>;
    };

    template <typename T>
    class IndexedSlotMap final {
    public:
        static constexpr std::size_t kInvalidIndex = static_cast<std::size_t>(-1);

        std::size_t insertWithId(std::uint64_t id, T value) {
            if constexpr (HasSlotId<T>) {
                value.id = id;
            }
            std::size_t const index = m_values.size();
            m_index[id] = index;
            m_values.push_back(Entry{id, std::move(value)});
            return index;
        }

        T* find(std::uint64_t id) {
            return const_cast<T*>(std::as_const(*this).find(id));
        }

        T const* find(std::uint64_t id) const {
            auto it = m_index.find(id);
            if (it == m_index.end()) {
                return nullptr;
            }
            std::size_t const index = it->second;
            if (index >= m_values.size()) {
                return nullptr;
            }
            Entry const& entry = m_values[index];
            if (entry.id != id) {
                return nullptr;
            }
            return &entry.value;
        }

        void eraseAt(std::size_t index) {
            if (index >= m_values.size()) {
                return;
            }
            std::size_t const last = m_values.size() - 1;
            m_index.erase(m_values[index].id);
            if (index != last) {
                m_values[index] = std::move(m_values[last]);
                m_index[m_values[index].id] = index;
            }
            m_values.pop_back();
        }

        void clear() {
            m_values.clear();
            m_index.clear();
        }

        std::size_t size() const {
            return m_values.size();
        }

        bool empty() const {
            return m_values.empty();
        }

        T& operator[](std::size_t index) {
            return m_values[index].value;
        }

        T const& operator[](std::size_t index) const {
            return m_values[index].value;
        }

        std::uint64_t idAt(std::size_t index) const {
            return m_values[index].id;
        }

        template <typename Fn>
        void forEachIndexSnapshot(Fn&& fn) {
            if (m_values.empty()) {
                return;
            }
            std::vector<std::uint64_t> ids;
            ids.reserve(m_values.size());
            for (Entry const& entry : m_values) {
                ids.push_back(entry.id);
            }
            for (std::uint64_t id : ids) {
                auto it = m_index.find(id);
                if (it == m_index.end()) {
                    continue;
                }
                std::size_t const index = it->second;
                if (index >= m_values.size() || m_values[index].id != id) {
                    continue;
                }
                fn(index, m_values[index].value);
            }
        }

#if defined(LUAUAPI_HOST_TESTS)
        bool contains(std::uint64_t id) const {
            return find(id) != nullptr;
        }

        std::size_t indexOf(std::uint64_t id) const {
            auto it = m_index.find(id);
            if (it == m_index.end()) {
                return kInvalidIndex;
            }
            std::size_t const index = it->second;
            if (index >= m_values.size() || m_values[index].id != id) {
                return kInvalidIndex;
            }
            return index;
        }
#endif

    private:
        struct Entry {
            std::uint64_t id = 0;
            T value{};
        };

        std::vector<Entry> m_values;
        std::unordered_map<std::uint64_t, std::size_t> m_index;
    };
} // namespace luax
