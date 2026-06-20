include(${CMAKE_CURRENT_LIST_DIR}/ImGui.cmake)

set(LUAUAPI_IMGUI_COCOS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/gd-imgui-cocos"
    CACHE PATH "Vendored gd-imgui-cocos source directory")
set(IMGUI_VERSION ${LUAUAPI_IMGUI_VERSION} CACHE STRING "What imgui version to download and use" FORCE)

if (NOT EXISTS "${LUAUAPI_IMGUI_COCOS_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "gd-imgui-cocos not found at ${LUAUAPI_IMGUI_COCOS_SOURCE_DIR}")
endif()

add_subdirectory(
    "${LUAUAPI_IMGUI_COCOS_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/gd-imgui-cocos"
)
target_include_directories(imgui-cocos INTERFACE "${PROJECT_SOURCE_DIR}/src")
message(STATUS "LuauAPI imgui-cocos: ${LUAUAPI_IMGUI_COCOS_SOURCE_DIR}")
message(STATUS "LuauAPI Dear ImGui: ${LUAUAPI_IMGUI_VERSION}")
