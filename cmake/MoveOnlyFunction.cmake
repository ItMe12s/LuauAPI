include(FetchContent)

FetchContent_Declare(
    nontype_functional
    GIT_REPOSITORY https://github.com/geode-sdk/nontype_functional.git
    GIT_TAG        6d1d08c
)
FetchContent_MakeAvailable(nontype_functional)

function(luauapi_link_move_only_function TARGET)
    if(NOT TARGET ${TARGET} OR NOT TARGET std23::nontype_functional)
        return()
    endif()
    target_link_libraries(${TARGET} PRIVATE std23::nontype_functional)
endfunction()
