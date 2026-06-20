include(${CMAKE_CURRENT_LIST_DIR}/ImGui.cmake)

set(_luauapi_imgui_cocos_dir "${PROJECT_SOURCE_DIR}/gd-imgui-cocos")
if (NOT EXISTS "${_luauapi_imgui_cocos_dir}/CMakeLists.txt")
    message(FATAL_ERROR "gd-imgui-cocos not found at ${_luauapi_imgui_cocos_dir}")
endif()

add_subdirectory(
    "${_luauapi_imgui_cocos_dir}"
    "${CMAKE_CURRENT_BINARY_DIR}/gd-imgui-cocos"
)
target_include_directories(imgui-cocos INTERFACE "${PROJECT_SOURCE_DIR}/src")
message(STATUS "LuauAPI imgui-cocos: ${_luauapi_imgui_cocos_dir}")
