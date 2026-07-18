#include "framework/stack/ContainerTables.hpp"
#include "host/lua_test_helpers.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <lua.h>
#include <string>
#include <tuple>
#include <utility>

namespace {
    template <class T>
    int checkContainerValue(lua_State* L) {
        static_cast<void>(luax::checkContainerValue<T>(L, 1, "value"));
        return 0;
    }

    template <class T>
    bool rejectsContainerValue(lua_State* L, int idx) {
        idx = lua_absindex(L, idx);
        lua_pushcfunction(L, &checkContainerValue<T>, "checkContainerValue");
        lua_pushvalue(L, idx);
        int const status = lua_pcall(L, 1, 0, 0);
        if (status != 0) {
            lua_pop(L, 1);
        }
        return status != 0;
    }
} // namespace

TEST_CASE("ContainerTables round-trips recursive map vector values") {
    auto state = luauapi_test::makeLuaState();
    auto* L = state.get();
    using Value = gd::map<int, gd::vector<int>>;

    Value input;
    input.emplace(4, gd::vector<int>{1, 2, 3});
    input.emplace(9, gd::vector<int>{});

    luax::pushContainerValue(L, input);
    REQUIRE(lua_istable(L, -1));
    REQUIRE(luax::checkContainerValue<Value>(L, -1, "mapVector") == input);
}

TEST_CASE("ContainerTables round-trips pair-key and unordered containers") {
    auto state = luauapi_test::makeLuaState();
    auto* L = state.get();

    using PairKeyValue = gd::map<std::pair<int, int>, gd::vector<int>>;
    PairKeyValue pairKeyInput;
    pairKeyInput.emplace(std::pair{1, 2}, gd::vector<int>{3, 4});
    luax::pushContainerValue(L, pairKeyInput);
    REQUIRE(luax::checkContainerValue<PairKeyValue>(L, -1, "pairKey") == pairKeyInput);
    lua_pop(L, 1);

    using UnorderedValue = gd::unordered_map<int, gd::unordered_set<int>>;
    UnorderedValue unorderedInput;
    unorderedInput.emplace(5, gd::unordered_set<int>{6, 7});
    luax::pushContainerValue(L, unorderedInput);
    REQUIRE(luax::checkContainerValue<UnorderedValue>(L, -1, "unordered") == unorderedInput);
}

TEST_CASE("ContainerTables round-trips mixed recursive values") {
    auto state = luauapi_test::makeLuaState();
    auto* L = state.get();
    using Value =
        gd::vector<std::tuple<int, bool, std::array<gd::vector<int>, 2>, gd::map<int, gd::vector<int>>>>;

    std::array<gd::vector<int>, 2> arrays = {
        gd::vector<int>{1, 2},
        gd::vector<int>{3},
    };
    gd::map<int, gd::vector<int>> mapping;
    mapping.emplace(5, gd::vector<int>{6, 7});

    Value input;
    input.emplace_back(8, true, std::move(arrays), std::move(mapping));

    luax::pushContainerValue(L, input);
    REQUIRE(luax::checkContainerValue<Value>(L, -1, "mixed") == input);
}

TEST_CASE("ContainerTables assigns pure values without whole-container assignment") {
    using Value = gd::map<int, gd::vector<int>>;
    Value destination;
    destination.emplace(1, gd::vector<int>{2});
    Value incoming;
    incoming.emplace(3, gd::vector<int>{4, 5});

    luax::assignContainerValue(destination, std::move(incoming));

    Value expected;
    expected.emplace(3, gd::vector<int>{4, 5});
    REQUIRE(destination == expected);
    REQUIRE(incoming.at(3) == gd::vector<int>{4, 5});
}

TEST_CASE("ContainerTables adapts SDK C-array fields") {
    auto state = luauapi_test::makeLuaState();
    auto* L = state.get();
    using Row = std::array<int, 2>;
    using Array = std::array<Row, 2>;
    int raw[2][2] = {{1, 2}, {3, 4}};

    luax::pushContainerValue<Array>(L, raw, nullptr);
    Array const expected{Row{1, 2}, Row{3, 4}};
    REQUIRE(luax::checkContainerValue<Array>(L, -1, "rawArray") == expected);

    luax::assignContainerValue(raw, Array{Row{5, 6}, Row{7, 8}});
    REQUIRE(raw[0][0] == 5);
    REQUIRE(raw[0][1] == 6);
    REQUIRE(raw[1][0] == 7);
    REQUIRE(raw[1][1] == 8);
}

TEST_CASE("ContainerTables tuples enforce arity positions and nested types") {
    auto state = luauapi_test::makeLuaState();
    auto* L = state.get();
    std::tuple<> const empty;
    luax::pushContainerValue(L, empty);
    REQUIRE(luax::checkContainerValue<std::tuple<>>(L, -1, "emptyTuple") == empty);
    lua_pop(L, 1);

    using Value = std::tuple<int, bool, std::string, gd::vector<int>>;
    Value const input{4, true, "ok", gd::vector<int>{7, 8}};

    luax::pushContainerValue(L, input);
    REQUIRE(luax::checkContainerValue<Value>(L, -1, "tuple") == input);

    SECTION("extra indexed values") {
        lua_pushinteger(L, 9);
        lua_rawseti(L, -2, 5);
        REQUIRE(rejectsContainerValue<Value>(L, -1));
    }

    SECTION("holes") {
        lua_pushnil(L);
        lua_rawseti(L, -2, 2);
        REQUIRE(rejectsContainerValue<Value>(L, -1));
    }

    SECTION("wrong nested element types") {
        lua_createtable(L, 1, 0);
        lua_pushliteral(L, "bad");
        lua_rawseti(L, -2, 1);
        lua_rawseti(L, -2, 4);
        REQUIRE(rejectsContainerValue<Value>(L, -1));
    }
}

TEST_CASE("ContainerTables keeps audited primitive pointer grids") {
    auto state = luauapi_test::makeLuaState();
    auto* L = state.get();
    using Row = gd::vector<int>;
    using Grid = gd::vector<Row*>;

    Row row{1, 2};
    Grid input{&row, nullptr};
    luax::pushAuditedPointerGrid(L, input);

    auto checked = luax::checkAuditedPointerGrid<Grid>(L, -1, "grid");
    REQUIRE(checked.size() == 2);
    REQUIRE(checked[0] == row);
    REQUIRE(checked[1].empty());

    Grid destination{new Row{9}};
    auto* const first = destination[0];
    luax::assignAuditedPointerGrid(destination, std::move(checked));
    REQUIRE(destination.size() == 2);
    REQUIRE(destination[0] == first);
    REQUIRE(*destination[0] == row);
    REQUIRE(destination[1] != nullptr);
    REQUIRE(destination[1]->empty());

    decltype(checked) grown{Row{3}, Row{4}, Row{5}};
    auto* const second = destination[1];
    luax::assignAuditedPointerGrid(destination, std::move(grown));
    REQUIRE(destination.size() == 3);
    REQUIRE(destination[0] == first);
    REQUIRE(destination[1] == second);
    REQUIRE(*destination[2] == Row{5});

    decltype(checked) shrunk{Row{6}};
    luax::assignAuditedPointerGrid(destination, std::move(shrunk));
    REQUIRE(destination.size() == 1);
    REQUIRE(destination[0] == first);
    REQUIRE(*destination[0] == Row{6});

    decltype(checked) empty;
    luax::assignAuditedPointerGrid(destination, std::move(empty));
    REQUIRE(destination.empty());

    Grid* nullGrid = nullptr;
    luax::pushAuditedPointerGrid(L, nullGrid);
    REQUIRE(lua_isnil(L, -1));
}

TEST_CASE("ContainerTables keeps audited object pointer grids") {
    auto state = luauapi_test::makeLuaState();
    auto* L = state.get();
    using Row = gd::vector<cocos2d::CCObject*>;
    using Grid = gd::vector<Row*>;
    using Layer = gd::vector<Row*>;
    using Grid3 = gd::vector<Layer*>;
    auto* owner = new cocos2d::CCObject();

    Row row;
    Grid input{&row, nullptr};
    luax::pushAuditedPointerGrid(L, input, owner);

    auto checked = luax::checkAuditedPointerGrid<Grid>(L, -1, "objectGrid");
    REQUIRE(checked.size() == 2);
    REQUIRE(checked[0].empty());
    REQUIRE(checked[1].empty());

    Grid destination{new Row};
    auto* const first = destination[0];
    luax::assignAuditedPointerGrid(destination, std::move(checked));
    REQUIRE(destination.size() == 2);
    REQUIRE(destination[0] == first);
    REQUIRE(destination[1] != nullptr);

    Layer middle{&row};
    Grid3 grid{&middle, nullptr};
    luax::pushAuditedPointerGrid(L, grid, owner);

    auto checkedGrid = luax::checkAuditedPointerGrid<Grid3>(L, -1, "objectGrid3");
    REQUIRE(checkedGrid.size() == 2);
    REQUIRE(checkedGrid[0].at(0).empty());
    REQUIRE(checkedGrid[1].empty());

    Grid3 destination3{new Layer{new Row}};
    auto* const firstLayer = destination3[0];
    auto* const firstRow = destination3[0]->at(0);
    luax::assignAuditedPointerGrid(destination3, std::move(checkedGrid));
    REQUIRE(destination3.size() == 2);
    REQUIRE(destination3[0] == firstLayer);
    REQUIRE(destination3[0]->at(0) == firstRow);
    REQUIRE(destination3[1] != nullptr);
    REQUIRE(destination3[1]->empty());

    decltype(checked) emptyRows;
    luax::assignAuditedPointerGrid(destination, std::move(emptyRows));
    decltype(checkedGrid) emptyGrid;
    luax::assignAuditedPointerGrid(destination3, std::move(emptyGrid));
    REQUIRE(destination.empty());
    REQUIRE(destination3.empty());
    owner->release();
}
