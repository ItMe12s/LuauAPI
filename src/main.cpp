#include "lua/Runtime.hpp"

#include <Geode/Geode.hpp>

#include <thread>

$on_mod(Loaded) {
    luax::Runtime::setMainThreadId(std::this_thread::get_id());
}
