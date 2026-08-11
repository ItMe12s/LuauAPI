set(LUAUAPI_BINDINGS_GIT_TAG "7431f0cf02fac40e6fa04faf30a7e248a6fee60d"
    CACHE STRING "Pinned bindings commit")
FetchContent_Declare(
    geode_bindings
    GIT_REPOSITORY https://github.com/geode-sdk/bindings.git
    GIT_TAG        ${LUAUAPI_BINDINGS_GIT_TAG}
    SOURCE_SUBDIR  _fetch_only_
)
FetchContent_MakeAvailable(geode_bindings)
message(STATUS "LuauAPI geode_bindings: ${LUAUAPI_BINDINGS_GIT_TAG}")
