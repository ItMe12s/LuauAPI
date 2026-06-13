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
include(cmake/GeodeBindings.cmake)

if (NOT LUAUAPI_HOST_ONLY)
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
    include(cmake/WebSocketDeps.cmake)
endif()

find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)

if (LUAUAPI_HOST_ONLY)
    message(STATUS "LuauAPI host-only configure: skipping Geode SDK and configure-time codegen listing")
endif()

if (NOT LUAUAPI_HOST_ONLY)
if (WIN32)
    set(LUAUAPI_CODEGEN_PLATFORM "win")
elseif (ANDROID)
    set(LUAUAPI_ANDROID_ABI "${CMAKE_ANDROID_ARCH_ABI}")
    if (NOT LUAUAPI_ANDROID_ABI AND DEFINED ANDROID_ABI)
        set(LUAUAPI_ANDROID_ABI "${ANDROID_ABI}")
    endif()

    if (GEODE_TARGET_PLATFORM STREQUAL "Android64" OR LUAUAPI_ANDROID_ABI STREQUAL "arm64-v8a")
        set(LUAUAPI_CODEGEN_PLATFORM "android64")
    elseif (GEODE_TARGET_PLATFORM STREQUAL "Android32" OR LUAUAPI_ANDROID_ABI STREQUAL "armeabi-v7a")
        set(LUAUAPI_CODEGEN_PLATFORM "android32")
    else()
        message(FATAL_ERROR "Unsupported Android ABI for LuauAPI codegen: ${LUAUAPI_ANDROID_ABI}")
    endif()
elseif (IOS)
    set(LUAUAPI_CODEGEN_PLATFORM "ios")
elseif (APPLE AND CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
    set(LUAUAPI_CODEGEN_PLATFORM "m1")
elseif (APPLE)
    set(LUAUAPI_CODEGEN_PLATFORM "imac")
else()
    message(WARNING "LuauAPI: unrecognized host platform, defaulting codegen platform to \"win\"")
    set(LUAUAPI_CODEGEN_PLATFORM "win")
endif()

set(LUAUAPI_GD_KEY "${LUAUAPI_CODEGEN_PLATFORM}")
if (LUAUAPI_CODEGEN_PLATFORM MATCHES "^android")
    set(LUAUAPI_GD_KEY "android")
elseif (LUAUAPI_CODEGEN_PLATFORM STREQUAL "m1" OR LUAUAPI_CODEGEN_PLATFORM STREQUAL "imac")
    set(LUAUAPI_GD_KEY "mac")
endif()

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/mod.json LUAUAPI_MOD_JSON)
string(REGEX MATCH "\"${LUAUAPI_GD_KEY}\"[ \t\r\n]*:[ \t\r\n]*\"([^\"]+)\"" LUAUAPI_GD_MATCH "${LUAUAPI_MOD_JSON}")
if (LUAUAPI_GD_MATCH)
    set(LUAUAPI_GD_VERSION "${CMAKE_MATCH_1}")
else()
    string(REGEX MATCH "\"win\"[ \t\r\n]*:[ \t\r\n]*\"([^\"]+)\"" LUAUAPI_GD_FALLBACK_MATCH "${LUAUAPI_MOD_JSON}")
    if (LUAUAPI_GD_FALLBACK_MATCH)
        message(WARNING "LuauAPI: gd.${LUAUAPI_GD_KEY} missing from mod.json, falling back to gd.win")
        set(LUAUAPI_GD_VERSION "${CMAKE_MATCH_1}")
    else()
        set(LUAUAPI_GD_VERSION "2.2081")
    endif()
endif()

set(LUAUAPI_BINDINGS_DIR
    "${geode_bindings_SOURCE_DIR}/bindings/${LUAUAPI_GD_VERSION}"
    CACHE PATH "Path to Geode bindings/<gdver>/ dir")
set(LUAUAPI_GEN_DIR "${CMAKE_BINARY_DIR}/luauapi-gen")
set(LUAUAPI_TYPES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/types")
set(LUAUAPI_DELEGATE_SPECS_OUT
    "${LUAUAPI_GEN_DIR}/delegate_specs.py")
set(LUAUAPI_CODEGEN_STAMP "${LUAUAPI_GEN_DIR}/codegen.stamp")
file(TO_CMAKE_PATH "$ENV{GEODE_SDK}" LUAUAPI_GEODE_SDK_PATH)

if (NOT EXISTS "${LUAUAPI_BINDINGS_DIR}/GeometryDash.bro")
    message(FATAL_ERROR "LuauAPI bindings dir missing: ${LUAUAPI_BINDINGS_DIR}")
endif()

file(GLOB_RECURSE LUAUAPI_CODEGEN_PY CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/luau_codegen/**/*.py)
file(GLOB LUAUAPI_BRO_FILES CONFIGURE_DEPENDS
    ${LUAUAPI_BINDINGS_DIR}/*.bro)
file(GLOB LUAUAPI_EXTRA_DLUAU_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/luau_codegen/extra_bindings/*.dluau)
file(GLOB LUAUAPI_GEODE_UI_HEADERS CONFIGURE_DEPENDS
    "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/ui/*.hpp")
set(LUAUAPI_GEODE_UI_MANIFEST
    "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/UI.hpp")
set(LUAUAPI_GEODE_COCOS_HEADERS
    "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/cocos/base_nodes/CCNode.h")
set(LUAUAPI_GEODE_UTILS_HEADERS
    "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/utils/general.hpp"
    "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/utils/string.hpp"
    "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/utils/random.hpp"
    "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/utils/cocos.hpp")

set(LUAUAPI_CODEGEN_CMD
    ${CMAKE_COMMAND} -E env "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/tools"
    ${Python3_EXECUTABLE} -m luau_codegen)

execute_process(
    COMMAND ${LUAUAPI_CODEGEN_CMD}
            --bindings ${LUAUAPI_BINDINGS_DIR}
            --list-all-outputs
            --platform ${LUAUAPI_CODEGEN_PLATFORM}
            --geode-sdk "${LUAUAPI_GEODE_SDK_PATH}"
    RESULT_VARIABLE LUAUAPI_LIST_ALL_OUTPUTS_RESULT
    OUTPUT_VARIABLE LUAUAPI_LIST_ALL_OUTPUTS_STDOUT
    ERROR_VARIABLE LUAUAPI_LIST_ALL_OUTPUTS_STDERR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if (NOT LUAUAPI_LIST_ALL_OUTPUTS_RESULT EQUAL 0)
    message(FATAL_ERROR "LuauAPI codegen output listing failed: ${LUAUAPI_LIST_ALL_OUTPUTS_STDERR}")
endif()
set(LUAUAPI_CODEGEN_BINDING_BYPRODUCTS)
set(LUAUAPI_GENERATED_TYPE_FILES)
if (LUAUAPI_LIST_ALL_OUTPUTS_STDOUT)
    string(REPLACE "\n" ";" LUAUAPI_LIST_ALL_OUTPUTS_LINES "${LUAUAPI_LIST_ALL_OUTPUTS_STDOUT}")
    foreach(LUAUAPI_LIST_ALL_OUTPUTS_LINE IN LISTS LUAUAPI_LIST_ALL_OUTPUTS_LINES)
        if (LUAUAPI_LIST_ALL_OUTPUTS_LINE MATCHES "^binding:(.+)$")
            list(APPEND LUAUAPI_CODEGEN_BINDING_BYPRODUCTS
                "${LUAUAPI_GEN_DIR}/${CMAKE_MATCH_1}")
        elseif (LUAUAPI_LIST_ALL_OUTPUTS_LINE MATCHES "^type:(.+)$")
            list(APPEND LUAUAPI_GENERATED_TYPE_FILES
                "${LUAUAPI_TYPES_DIR}/${CMAKE_MATCH_1}")
        endif()
    endforeach()
endif()

set(LUAUAPI_GENERATED_METADATA_FILES
    "${LUAUAPI_GEN_DIR}/schema.json"
    "${LUAUAPI_GEN_DIR}/report.md"
    "${LUAUAPI_GEN_DIR}/parity.json"
    "${LUAUAPI_GEN_DIR}/audit.md")

add_custom_command(
    OUTPUT ${LUAUAPI_CODEGEN_STAMP}
    BYPRODUCTS
            ${LUAUAPI_CODEGEN_BINDING_BYPRODUCTS}
            ${LUAUAPI_GENERATED_TYPE_FILES}
            ${LUAUAPI_GENERATED_METADATA_FILES}
            ${LUAUAPI_DELEGATE_SPECS_OUT}
    COMMAND ${LUAUAPI_CODEGEN_CMD}
            --bindings ${LUAUAPI_BINDINGS_DIR}
            --out ${LUAUAPI_GEN_DIR}
            --types-out ${LUAUAPI_TYPES_DIR}
            --delegate-specs-out ${LUAUAPI_DELEGATE_SPECS_OUT}
            --platform ${LUAUAPI_CODEGEN_PLATFORM}
            --geode-sdk "${LUAUAPI_GEODE_SDK_PATH}"
    COMMAND ${CMAKE_COMMAND} -E touch ${LUAUAPI_CODEGEN_STAMP}
    DEPENDS
        ${LUAUAPI_CODEGEN_PY}
        ${LUAUAPI_BRO_FILES}
        ${LUAUAPI_EXTRA_DLUAU_FILES}
        ${LUAUAPI_GEODE_UI_HEADERS}
        ${LUAUAPI_GEODE_UI_MANIFEST}
        ${LUAUAPI_GEODE_COCOS_HEADERS}
        ${LUAUAPI_GEODE_UTILS_HEADERS}
    COMMENT "LuauAPI Broma codegen"
    VERBATIM
)
add_custom_target(luauapi_codegen
    DEPENDS
        ${LUAUAPI_CODEGEN_STAMP}
        ${LUAUAPI_CODEGEN_BINDING_BYPRODUCTS}
        ${LUAUAPI_GENERATED_TYPE_FILES}
        ${LUAUAPI_GENERATED_METADATA_FILES})

set(LUAUAPI_GENERATED_BINDING_SOURCES ${LUAUAPI_CODEGEN_BINDING_BYPRODUCTS})
list(FILTER LUAUAPI_GENERATED_BINDING_SOURCES INCLUDE REGEX "\\.cpp$")
set_source_files_properties(${LUAUAPI_GENERATED_BINDING_SOURCES} PROPERTIES GENERATED TRUE)

file(GLOB_RECURSE LUAUAPI_HANDWRITTEN_SOURCES CONFIGURE_DEPENDS src/*.cpp)
list(FILTER LUAUAPI_HANDWRITTEN_SOURCES EXCLUDE REGEX "LuaDelegates\\.gen\\.cpp$")
set(LUAUAPI_RUNTIME_SOURCES ${LUAUAPI_HANDWRITTEN_SOURCES})
list(FILTER LUAUAPI_RUNTIME_SOURCES EXCLUDE REGEX "[/\\\\]main\\.cpp$")

list(LENGTH LUAUAPI_GENERATED_BINDING_SOURCES LUAUAPI_GENERATED_BINDING_COUNT)
message(STATUS "LuauAPI GD bindings version: ${LUAUAPI_GD_VERSION} (platform ${LUAUAPI_CODEGEN_PLATFORM}, ${LUAUAPI_GENERATED_BINDING_COUNT} generated binding sources)")

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

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER MATCHES "clang-cl")
    set_source_files_properties(${LUAUAPI_GENERATED_BINDING_SOURCES} PROPERTIES COMPILE_OPTIONS "/w")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set_source_files_properties(${LUAUAPI_GENERATED_BINDING_SOURCES} PROPERTIES COMPILE_OPTIONS "-w")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER MATCHES "clang-cl")
    target_compile_options(LuauAPI_GeneratedBindings PRIVATE $<$<NOT:$<CONFIG:Debug>>:/O1>)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(LuauAPI_GeneratedBindings PRIVATE $<$<NOT:$<CONFIG:Debug>>:-Oz>)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(LuauAPI_GeneratedBindings PRIVATE $<$<NOT:$<CONFIG:Debug>>:-Os>)
endif()

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
    include(cmake/WebSocketDeps.cmake)
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
        GIT_TAG        12.1.0
    )
    FetchContent_MakeAvailable(fmt)

    file(GLOB LUAUAPI_TEST_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)

    add_executable(luauapi_tests
        src/api.cpp
        src/framework/Binding.cpp
        src/framework/BindingHost.cpp
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
        src/bindings/geode/GeodeJsonBinding.cpp
        src/bindings/geode/JsonConvert.cpp
        src/bindings/imgui/ImGuiDrawHandleBinding.cpp
        src/bindings/imgui/ImGuiDrawScheduler.cpp
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
    target_link_libraries(luauapi_tests PRIVATE
        Catch2::Catch2WithMain
        fmt::fmt
        glm::glm
        Luau.VM
        Luau.Compiler
        Luau.CodeGen
        Luau.Require
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
