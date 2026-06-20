cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(LuauAPI)

include(FetchContent)
include(CTest)

option(LUAUAPI_BUILD_TESTS "Build LuauAPI host unit tests" OFF)
option(LUAUAPI_HOST_ONLY "Host-only configure without Geode SDK or configure-time codegen listing" OFF)

include(cmake/Helpers.cmake)
include(cmake/Luau.cmake)
include(cmake/Glm.cmake)

if (NOT LUAUAPI_HOST_ONLY)
    include(cmake/GeodeBindings.cmake)

    if (NOT DEFINED ENV{GEODE_SDK})
        message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
    else()
        message(STATUS "Found Geode: $ENV{GEODE_SDK}")
    endif()

    if (NOT DEFINED ENV{GEODE_BINDINGS_REPO_PATH} AND NOT GEODE_BINDINGS_REPO_PATH)
        set(GEODE_BINDINGS_REPO_PATH "${geode_bindings_SOURCE_DIR}"
            CACHE PATH "Geode bindings repo (LuauAPI LUAUAPI_BINDINGS_GIT_TAG pin)" FORCE)
        message(STATUS "LuauAPI GEODE_BINDINGS_REPO_PATH: ${GEODE_BINDINGS_REPO_PATH}")
    endif()

    add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

    include(cmake/ImGuiCocos.cmake)
endif()

if(NOT LUAUAPI_HOST_ONLY OR LUAUAPI_BUILD_TESTS)
    include(cmake/WebSocketDeps.cmake)
endif()

find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)

if (LUAUAPI_HOST_ONLY)
    message(STATUS "LuauAPI host-only configure: skipping Geode SDK and configure-time codegen listing")
endif()

if (NOT LUAUAPI_HOST_ONLY)
    include(cmake/Codegen.cmake)
    luauapi_setup_codegen()

    file(GLOB_RECURSE LUAUAPI_HANDWRITTEN_SOURCES CONFIGURE_DEPENDS src/*.cpp)
    list(FILTER LUAUAPI_HANDWRITTEN_SOURCES EXCLUDE REGEX "LuaDelegates\\.gen\\.cpp$")
    set(LUAUAPI_RUNTIME_SOURCES ${LUAUAPI_HANDWRITTEN_SOURCES})
    list(FILTER LUAUAPI_RUNTIME_SOURCES EXCLUDE REGEX "[/\\\\]main\\.cpp$")

    add_library(LuauAPI_Runtime OBJECT ${LUAUAPI_RUNTIME_SOURCES})
    luauapi_apply_compile_settings(LuauAPI_Runtime)
    set_target_properties(LuauAPI_Runtime PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(LuauAPI_Runtime PUBLIC
        include
        src
        vendor
        ${LUAUAPI_GEN_DIR}/src
        $<TARGET_PROPERTY:imgui-cocos,INTERFACE_INCLUDE_DIRECTORIES>
    )
    add_dependencies(LuauAPI_Runtime luauapi_codegen)
    target_compile_definitions(LuauAPI_Runtime PUBLIC
        $<TARGET_PROPERTY:imgui-cocos,INTERFACE_COMPILE_DEFINITIONS>
    )
    target_link_libraries(LuauAPI_Runtime PUBLIC
        ${LUAUAPI_LUAU_LINK_LIBS}
        GeodeBindings
        ixwebsocket
        glm::glm
    )

    add_library(LuauAPI_GeneratedBindings OBJECT ${LUAUAPI_GENERATED_BINDING_SOURCES})
    add_dependencies(LuauAPI_GeneratedBindings luauapi_codegen)
    luauapi_apply_compile_settings(LuauAPI_GeneratedBindings)
    set_target_properties(LuauAPI_GeneratedBindings PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(LuauAPI_GeneratedBindings PUBLIC ${LUAUAPI_GEN_DIR}/src)
    target_link_libraries(LuauAPI_GeneratedBindings PUBLIC LuauAPI_Runtime)
    luauapi_apply_generated_binding_compile_settings(LuauAPI_GeneratedBindings)

    add_library(${PROJECT_NAME} SHARED
        src/main.cpp
        $<TARGET_OBJECTS:LuauAPI_Runtime>
        $<TARGET_OBJECTS:LuauAPI_GeneratedBindings>
    )
    target_include_directories(${PROJECT_NAME} PRIVATE include src ${LUAUAPI_GEN_DIR}/src)
    target_link_libraries(${PROJECT_NAME}
        ${LUAUAPI_LUAU_LINK_LIBS}
        imgui-cocos
        geode-sdk
        ixwebsocket
    )
    luauapi_apply_compile_settings(${PROJECT_NAME})
    setup_geode_mod(${PROJECT_NAME})

    if(WIN32)
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<NOT:$<CONFIG:Debug>>:LINKER:/OPT:REF>
            $<$<NOT:$<CONFIG:Debug>>:LINKER:/OPT:ICF>)
    endif()
endif()

if(LUAUAPI_BUILD_TESTS)
    include(cmake/ImGuiHeadless.cmake)
    enable_testing()

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.15.0
    )
    FetchContent_MakeAvailable(Catch2)

    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG        12.2.0
    )
    FetchContent_MakeAvailable(fmt)

    file(GLOB LUAUAPI_TEST_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)

    add_executable(luauapi_tests
        src/api.cpp
        src/framework/Binding.cpp
        src/framework/callback/LuaCocosHandler.cpp
        src/framework/usertype/Fields.cpp
        src/framework/usertype/OpaqueHandle.cpp
        src/framework/usertype/Usertype.cpp
        src/framework/usertype/UsertypeRegistry.cpp
        src/framework/callback/LuaDelegate.cpp
        src/framework/callback/LuaTrampolineRegistry.cpp
        src/bindings/task/TaskBinding.cpp
        src/bindings/render3d/Gd3dRegister.cpp
        src/bindings/render3d/TransformBinding.cpp
        src/bindings/render3d/ProceduralMeshBinding.cpp
        src/bindings/render3d/GltfBinding.cpp
        src/bindings/render3d/internal/MeshHandleBinding.cpp
        src/bindings/websocket/WebSocketBinding.cpp
        src/bindings/websocket/WebSocketConnection.cpp
        src/bindings/websocket/WebSocketServer.cpp
        src/bindings/geode/CurrentMod.cpp
        src/bindings/geode/ModSandbox.cpp
        src/bindings/geode/GeodeFsBinding.cpp
        src/bindings/geode/GeodeSmallBindings.cpp
        src/bindings/geode/web/GeodeWebCore.cpp
        src/bindings/geode/web/GeodeWebApi.cpp
        src/bindings/geode/web/GeodeWebListeners.cpp
        src/bindings/geode/JsonConvert.cpp
        src/bindings/imgui/ImGuiCore.cpp
        src/bindings/imgui/ImGuiWidgetsLayout.cpp
        src/bindings/imgui/ImGuiPopupsTablesMenus.cpp
        src/bindings/imgui/ImGuiStyleFonts.cpp
        tests/host/ImGuiHostStub.cpp
        src/bindings/task/TaskScheduler.cpp
        src/require/BytecodeCacheKey.cpp
        src/require/Requirer.cpp
        src/core/Loadstring.cpp
        src/core/Runtime.cpp
        src/render3d/assets/MeshAsset.cpp
        src/render3d/assets/TextureAsset.cpp
        src/render3d/assets/GltfIo.cpp
        src/render3d/assets/ImageDecode.cpp
        src/render3d/gpu/SceneDrawList.cpp
        tests/host/Render3DMathTests.cpp
        tests/host/GltfParseTests.cpp
        tests/host/ImageDecodeTests.cpp
        tests/host/ProceduralMeshTests.cpp
        tests/host/Gd3dTransformBindingTests.cpp
        tests/host/Gd3dMeshBindingTests.cpp
        tests/host/FrustumTests.cpp
        tests/host/SceneDrawListTests.cpp
        ${LUAUAPI_TEST_SOURCES}
    )
    luauapi_force_msvc_idl0(luauapi_tests Catch2 Catch2WithMain luauapi_imgui_headless)
    target_compile_features(luauapi_tests PRIVATE cxx_std_23)
    target_include_directories(luauapi_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/include
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/host
        src
        include
        vendor
    )
    target_compile_definitions(luauapi_tests PRIVATE LUAUAPI_HOST_TESTS=1)
    target_compile_definitions(luauapi_tests PRIVATE
        "LUAUAPI_TEST_FONT_PATH=\"${imgui_headless_SOURCE_DIR}/misc/fonts/DroidSans.ttf\"")
    target_link_libraries(luauapi_tests PRIVATE
        Catch2::Catch2WithMain
        fmt::fmt-header-only
        glm::glm
        Luau.VM
        Luau.Compiler
        Luau.CodeGen
        Luau.Require
        luauapi_imgui_headless
        ixwebsocket
        mbedtls
        mbedx509
        mbedcrypto
    )
    if(WIN32)
        target_link_libraries(luauapi_tests PRIVATE ws2_32)
    endif()

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    catch_discover_tests(luauapi_tests DISCOVERY_MODE PRE_TEST)
    add_test(
        NAME luauapi_codegen_tests
        COMMAND ${Python3_EXECUTABLE} -m unittest discover -s tests/luau_codegen -p "test_*.py"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    set_tests_properties(luauapi_codegen_tests PROPERTIES
        ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/tools")
endif()
