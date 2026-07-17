set(LUAUAPI_BINDINGS_GIT_TAG "ba9f177be0e428461160c1c9d6c0414831ffea0e"
    CACHE STRING "Pinned bindings commit")
FetchContent_Declare(
    geode_bindings
    GIT_REPOSITORY https://github.com/geode-sdk/bindings.git
    GIT_TAG        ${LUAUAPI_BINDINGS_GIT_TAG}
    SOURCE_SUBDIR  _fetch_only_
)
FetchContent_MakeAvailable(geode_bindings)
message(STATUS "LuauAPI geode_bindings: ${LUAUAPI_BINDINGS_GIT_TAG}")
