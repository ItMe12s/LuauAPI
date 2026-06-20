cmake_minimum_required(VERSION 3.14)

project(imgui-cocos)

if (NOT DEFINED imgui_SOURCE_DIR)
    message(FATAL_ERROR "Include cmake/ImGui.cmake before add_subdirectory(gd-imgui-cocos)")
endif()
add_library(imgui-cocos INTERFACE)
set_target_properties(imgui-cocos PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_compile_definitions(imgui-cocos INTERFACE IMGUI_COCOS_HOOK_EARLY=1)

target_include_directories(imgui-cocos INTERFACE
    include
    ${imgui_SOURCE_DIR}
)

target_sources(imgui-cocos INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hooks.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
)
