include(${CMAKE_CURRENT_LIST_DIR}/ImGui.cmake)

FetchContent_Declare(
    imgui_headless
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG ${LUAUAPI_IMGUI_VERSION}
)
FetchContent_MakeAvailable(imgui_headless)

add_library(luauapi_imgui_headless STATIC
    ${imgui_headless_SOURCE_DIR}/imgui.cpp
    ${imgui_headless_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_headless_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_headless_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_headless_SOURCE_DIR}/backends/imgui_impl_null.cpp
)
target_include_directories(luauapi_imgui_headless PUBLIC ${imgui_headless_SOURCE_DIR})
