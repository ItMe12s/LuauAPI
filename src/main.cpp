#include "lua/Runtime.hpp"

#include <Geode/Geode.hpp>
#include <Geode/loader/GameEvent.hpp>

#include <thread>

$execute {
    luax::Runtime::setMainThreadId(std::this_thread::get_id());
}

$on_mod(Loaded) {
    luax::Runtime::getOrCreate();
}

$on_game(Exiting) {
    luax::Runtime::shutdown();
}
