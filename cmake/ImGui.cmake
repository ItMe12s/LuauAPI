set(LUAUAPI_IMGUI_VERSION "v1.92.8"
    CACHE STRING "Pinned Dear ImGui release")

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG ${LUAUAPI_IMGUI_VERSION}
)
FetchContent_MakeAvailable(imgui)
message(STATUS "LuauAPI Dear ImGui: ${LUAUAPI_IMGUI_VERSION}")
