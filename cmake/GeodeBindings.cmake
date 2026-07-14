set(LUAUAPI_BINDINGS_GIT_TAG "2a8b5c489ce8b49e7061b0543aa2bc5b22570063"
    CACHE STRING "Pinned bindings commit")
FetchContent_Declare(
    geode_bindings
    GIT_REPOSITORY https://github.com/geode-sdk/bindings.git
    GIT_TAG        ${LUAUAPI_BINDINGS_GIT_TAG}
    SOURCE_SUBDIR  _fetch_only_
)
FetchContent_MakeAvailable(geode_bindings)
message(STATUS "LuauAPI geode_bindings: ${LUAUAPI_BINDINGS_GIT_TAG}")
