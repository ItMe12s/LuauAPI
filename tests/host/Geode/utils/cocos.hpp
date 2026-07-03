#pragma once

#include <Geode/Result.hpp>
#include <cocos2d.h>
#include <string>
#include <string_view>
#include <vector>

namespace gd {
    template <class T>
    using vector = std::vector<T>;
}

namespace geode::cocos {
    void setObjectNameForTests(cocos2d::CCObject const* obj, std::string name);
    void clearObjectNamesForTests();
    std::string_view getObjectName(cocos2d::CCObject const* obj);
} // namespace geode::cocos
