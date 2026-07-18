#pragma once

#include <Geode/Result.hpp>
#include <cocos2d.h>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gd {
    using string = std::string;

    template <class T>
    using vector = std::vector<T>;

    template <class K, class V>
    using map = std::map<K, V>;

    template <class K, class V>
    using unordered_map = std::unordered_map<K, V>;

    template <class T>
    using set = std::set<T>;

    template <class T>
    using unordered_set = std::unordered_set<T>;
} // namespace gd

namespace geode::cocos {
    void setObjectNameForTests(cocos2d::CCObject const* obj, std::string name);
    void clearObjectNamesForTests();
    std::string_view getObjectName(cocos2d::CCObject const* obj);
} // namespace geode::cocos
