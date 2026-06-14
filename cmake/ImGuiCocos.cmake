set(LUAUAPI_IMGUI_COCOS_GIT_TAG "b93f08ccef778a53ebba09b20c347f6a63980119"
    CACHE STRING "Pinned gd-imgui-cocos commit")
set(LUAUAPI_IMGUI_VERSION "v1.92.8"
    CACHE STRING "Pinned Dear ImGui release fetched by gd-imgui-cocos")
set(IMGUI_VERSION ${LUAUAPI_IMGUI_VERSION} CACHE STRING "What imgui version to download and use" FORCE)
FetchContent_Declare(
    gd_imgui_cocos
    GIT_REPOSITORY https://github.com/matcool/gd-imgui-cocos.git
    GIT_TAG        ${LUAUAPI_IMGUI_COCOS_GIT_TAG}
)
FetchContent_MakeAvailable(gd_imgui_cocos)
message(STATUS "LuauAPI imgui-cocos: ${LUAUAPI_IMGUI_COCOS_GIT_TAG}")
message(STATUS "LuauAPI Dear ImGui: ${LUAUAPI_IMGUI_VERSION}")
