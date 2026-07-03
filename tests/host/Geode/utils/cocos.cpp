#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace geode::cocos {
    namespace {
        std::unordered_map<cocos2d::CCObject const*, std::string>& runtimeNamesForTests() {
            static std::unordered_map<cocos2d::CCObject const*, std::string> names;
            return names;
        }
    } // namespace

    void setObjectNameForTests(cocos2d::CCObject const* obj, std::string name) {
        if (!obj || name.empty()) {
            return;
        }
        runtimeNamesForTests().insert_or_assign(obj, std::move(name));
    }

    void clearObjectNamesForTests() {
        runtimeNamesForTests().clear();
    }

    std::string_view getObjectName(cocos2d::CCObject const* obj) {
        if (!obj) {
            return {};
        }
        auto it = runtimeNamesForTests().find(obj);
        if (it == runtimeNamesForTests().end()) {
            return {};
        }
        return it->second;
    }
} // namespace geode::cocos
