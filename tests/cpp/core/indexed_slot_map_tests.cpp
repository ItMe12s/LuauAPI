#include "core/IndexedSlotMap.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

namespace {
    struct SlotValue {
        std::uint64_t id = 0;
        int payload = 0;
        bool cancelled = false;
    };
} // namespace

TEST_CASE("IndexedSlotMap insert and find by stable id") {
    luax::IndexedSlotMap<SlotValue> map;

    map.insertWithId(1, SlotValue{.id = 1, .payload = 10});
    map.insertWithId(2, SlotValue{.id = 2, .payload = 20});

    REQUIRE(map.size() == 2);
    REQUIRE(map.find(1) != nullptr);
    REQUIRE(map.find(1)->payload == 10);
    REQUIRE(map.find(2) != nullptr);
    REQUIRE(map.find(2)->payload == 20);
    REQUIRE(map.find(99) == nullptr);
}

TEST_CASE("IndexedSlotMap eraseAt swap-and-pop keeps index valid") {
    luax::IndexedSlotMap<SlotValue> map;

    map.insertWithId(1, SlotValue{.id = 1, .payload = 1});
    map.insertWithId(2, SlotValue{.id = 2, .payload = 2});
    map.insertWithId(3, SlotValue{.id = 3, .payload = 3});

    map.eraseAt(1);
    REQUIRE(map.size() == 2);
    REQUIRE(map.indexOf(1) == 0);
    REQUIRE(map.indexOf(3) == 1);
    REQUIRE(map.find(2) == nullptr);
    REQUIRE(map.find(3) != nullptr);
    REQUIRE(map.find(3)->payload == 3);
}

TEST_CASE("IndexedSlotMap ids are not reused after erase") {
    luax::IndexedSlotMap<SlotValue> map;

    map.insertWithId(5, SlotValue{.id = 5, .payload = 5});
    map.eraseAt(0);
    map.insertWithId(6, SlotValue{.id = 6, .payload = 6});

    REQUIRE(map.find(5) == nullptr);
    REQUIRE(map.find(6) != nullptr);
    REQUIRE(map.contains(5) == false);
    REQUIRE(map.contains(6) == true);
}

TEST_CASE("IndexedSlotMap forEachIndexSnapshot tolerates erase during callback") {
    luax::IndexedSlotMap<SlotValue> map;

    map.insertWithId(1, SlotValue{.id = 1, .payload = 1});
    map.insertWithId(2, SlotValue{.id = 2, .payload = 2});
    map.insertWithId(3, SlotValue{.id = 3, .payload = 3});

    int visited = 0;
    map.forEachIndexSnapshot([&](std::size_t index, SlotValue& value) {
        ++visited;
        if (value.payload == 2) {
            map.eraseAt(index);
        }
    });

    REQUIRE(visited == 3);
    REQUIRE(map.size() == 2);
    REQUIRE(map.find(2) == nullptr);
    REQUIRE(map.find(1) != nullptr);
    REQUIRE(map.find(3) != nullptr);
}

TEST_CASE("IndexedSlotMap forEachIndexSnapshot skips entries added during callback") {
    luax::IndexedSlotMap<SlotValue> map;

    map.insertWithId(1, SlotValue{.id = 1, .payload = 1});

    int visited = 0;
    map.forEachIndexSnapshot([&](std::size_t, SlotValue&) {
        ++visited;
        map.insertWithId(2, SlotValue{.id = 2, .payload = 2});
    });

    REQUIRE(visited == 1);
    REQUIRE(map.size() == 2);
    REQUIRE(map.find(2) != nullptr);
}

TEST_CASE("IndexedSlotMap clear resets storage and lookup") {
    luax::IndexedSlotMap<SlotValue> map;

    map.insertWithId(1, SlotValue{.id = 1, .payload = 1});
    map.clear();

    REQUIRE(map.empty());
    REQUIRE(map.find(1) == nullptr);
}

TEST_CASE("IndexedSlotMap insertWithId replaces duplicate id in place") {
    luax::IndexedSlotMap<SlotValue> map;

    std::size_t firstIndex = map.insertWithId(7, SlotValue{.id = 7, .payload = 10});
    map.insertWithId(8, SlotValue{.id = 8, .payload = 20});

    std::size_t replacedIndex = map.insertWithId(7, SlotValue{.id = 7, .payload = 99});
    REQUIRE(replacedIndex == firstIndex);
    REQUIRE(map.size() == 2);
    REQUIRE(map.find(7) != nullptr);
    REQUIRE(map.find(7)->payload == 99);
    REQUIRE(map.find(8) != nullptr);
    REQUIRE(map.indexOf(7) == firstIndex);
}
