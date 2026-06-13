#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

namespace luax {
    template <class T>
    class WeakHandlePool {
    public:
        void track(std::shared_ptr<T> const& item) {
            items_.push_back(item);
        }

        void track(std::weak_ptr<T> weak) {
            items_.push_back(std::move(weak));
        }

        void compact() {
            (void)compactAndCountLive();
        }

        std::size_t compactAndCountLive() {
            items_.erase(
                std::remove_if(
                    items_.begin(),
                    items_.end(),
                    [](auto const& weak) {
                        return weak.expired();
                    }
                ),
                items_.end()
            );
            return items_.size();
        }

        template <class Fn>
        void clearAll(Fn&& onLive) {
            for (auto& weak : items_) {
                if (auto item = weak.lock()) {
                    onLive(*item);
                }
            }
            items_.clear();
        }

        [[nodiscard]] bool empty() const {
            return items_.empty();
        }

        [[nodiscard]] std::size_t size() const {
            return items_.size();
        }

    private:
        std::vector<std::weak_ptr<T>> items_;
    };
} // namespace luax
