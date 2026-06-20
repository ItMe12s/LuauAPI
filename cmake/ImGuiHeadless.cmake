include(${CMAKE_CURRENT_LIST_DIR}/ImGui.cmake)

add_library(luauapi_imgui_headless STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_null.cpp
)
target_include_directories(luauapi_imgui_headless PUBLIC ${imgui_SOURCE_DIR})
