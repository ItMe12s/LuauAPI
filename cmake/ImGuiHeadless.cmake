set(LUAUAPI_IMGUI_VERSION "v1.92.8"
    CACHE STRING "Dear ImGui release for headless host tests")

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
