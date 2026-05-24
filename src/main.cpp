#include "lua/Runtime.hpp"

#include <Geode/Geode.hpp>

#include <thread>

$execute {
    luax::Runtime::setMainThreadId(std::this_thread::get_id());
}
