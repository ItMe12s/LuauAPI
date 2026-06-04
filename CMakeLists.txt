cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 26)
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
option(LUAUAPI_STRICT_GENERATED_WARNINGS "Compile generated bindings with warnings enabled (use this for auditing stuff)" OFF)

set(LUAU_BUILD_CLI   OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_WEB   OFF CACHE BOOL "" FORCE)
set(LUAU_EXTERN_C    OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    luau
    GIT_REPOSITORY https://github.com/luau-lang/luau.git
    GIT_TAG        0.723
)
FetchContent_MakeAvailable(luau)

foreach(LUAU_TARGET
    Luau.Common
    Luau.Ast
    Luau.Bytecode
    Luau.Compiler
    Luau.Config
    Luau.Analysis
    Luau.CodeGen
    Luau.VM
    Luau.Require
)
    if (TARGET ${LUAU_TARGET})
        get_target_property(LUAU_PUBLIC_INCLUDES ${LUAU_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
        if (LUAU_PUBLIC_INCLUDES)
            set_target_properties(${LUAU_TARGET} PROPERTIES
                INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${LUAU_PUBLIC_INCLUDES}"
            )
        endif()
    endif()
endforeach()

set(LUAUAPI_BINDINGS_GIT_TAG "00328ccd2fd3e4b005a54eaaa4d4d91e22ca7df4"
    CACHE STRING "Pinned bindings commit")
FetchContent_Declare(
    geode_bindings
    GIT_REPOSITORY https://github.com/geode-sdk/bindings.git
    GIT_TAG        ${LUAUAPI_BINDINGS_GIT_TAG}
    SOURCE_SUBDIR  _fetch_only_
)
FetchContent_MakeAvailable(geode_bindings)
message(STATUS "LuauAPI geode_bindings: ${LUAUAPI_BINDINGS_GIT_TAG}")

# So basically what happened is that Luau and isocline ship without MSVC iterator-debug ABI.
# Geode/Cocos use the default debug iterator layout when _ITERATOR_DEBUG_LEVEL is 1.
# And I'm making sure Luau targets use iterator debug level 0 for STL compatibility.
if (WIN32)
    foreach(LUAU_TARGET
        Luau.Common
        Luau.Ast
        Luau.Bytecode
        Luau.Compiler
        Luau.Config
        Luau.Analysis
        Luau.CodeGen
        Luau.VM
        Luau.Require
        isocline
    )
        if (TARGET ${LUAU_TARGET})
            target_compile_definitions(${LUAU_TARGET} PRIVATE
                _HAS_ITERATOR_DEBUGGING=0
                _ITERATOR_DEBUG_LEVEL=0
            )
        endif()
    endforeach()
endif()

foreach(LUAU_TARGET Luau.VM Luau.CodeGen)
    if (TARGET ${LUAU_TARGET})
        # I could hardcode this.
        target_compile_definitions(${LUAU_TARGET} PUBLIC LUA_UTAG_LIMIT=2048)
    endif()
endforeach()

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
endif()

if (NOT LUAUAPI_HOST_ONLY)
    set(LUAUAPI_IMGUI_COCOS_GIT_TAG "b93f08ccef778a53ebba09b20c347f6a63980119"
        CACHE STRING "Pinned gd-imgui-cocos commit")
    FetchContent_Declare(
        gd_imgui_cocos
        GIT_REPOSITORY https://github.com/matcool/gd-imgui-cocos.git
        GIT_TAG        ${LUAUAPI_IMGUI_COCOS_GIT_TAG}
    )
    FetchContent_MakeAvailable(gd_imgui_cocos)
    message(STATUS "LuauAPI imgui-cocos: ${LUAUAPI_IMGUI_COCOS_GIT_TAG}")
endif()

find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)

if (LUAUAPI_HOST_ONLY)
    message(STATUS "LuauAPI host-only configure: skipping Geode SDK and configure-time codegen listing")
endif()

if (NOT LUAUAPI_HOST_ONLY)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/mod.json LUAUAPI_MOD_JSON)
string(REGEX MATCH "\"win\"[ \t\r\n]*:[ \t\r\n]*\"([^\"]+)\"" LUAUAPI_GD_MATCH "${LUAUAPI_MOD_JSON}")
if (CMAKE_MATCH_1)
    set(LUAUAPI_GD_VERSION "${CMAKE_MATCH_1}")
else()
    set(LUAUAPI_GD_VERSION "2.2081")
endif()

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
    set(LUAUAPI_CODEGEN_PLATFORM "win")
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
set(LUAUAPI_GEODE_COCOS_HEADERS
    "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/cocos/base_nodes/CCNode.h")
set(LUAUAPI_GEODE_UTILS_HEADERS
    "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/utils/general.hpp"
    "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/utils/string.hpp"
    "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/utils/random.hpp")

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

function(luauapi_apply_compile_settings TARGET)
    target_compile_features(${TARGET} PRIVATE cxx_std_23)
    target_compile_definitions(${TARGET} PRIVATE LUAUAPI_EXPORTING=1)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER MATCHES "clang-cl")
        target_compile_options(${TARGET} PRIVATE /W4)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic -Wno-dollar-in-identifier-extension)
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${TARGET} PRIVATE -Wno-gnu-zero-variadic-macro-arguments)
    endif()

    if(WIN32)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER MATCHES "clang-cl")
            target_compile_options(${TARGET} PRIVATE /bigobj)
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${TARGET} PRIVATE -Wa,-mbig-obj)
        endif()
    endif()
endfunction()

add_library(LuauAPI_Runtime OBJECT ${LUAUAPI_RUNTIME_SOURCES})
luauapi_apply_compile_settings(LuauAPI_Runtime)
set_target_properties(LuauAPI_Runtime PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(LuauAPI_Runtime PUBLIC
    include
    src
    $<TARGET_PROPERTY:imgui-cocos,INTERFACE_INCLUDE_DIRECTORIES>
)
target_compile_definitions(LuauAPI_Runtime PUBLIC
    $<TARGET_PROPERTY:imgui-cocos,INTERFACE_COMPILE_DEFINITIONS>
)
target_link_libraries(LuauAPI_Runtime PUBLIC
    Luau.VM
    Luau.Compiler
    Luau.Ast
    Luau.Require
    Luau.CodeGen
    GeodeBindings
)

add_library(LuauAPI_GeneratedBindings OBJECT ${LUAUAPI_GENERATED_BINDING_SOURCES})
add_dependencies(LuauAPI_GeneratedBindings luauapi_codegen)
luauapi_apply_compile_settings(LuauAPI_GeneratedBindings)
set_target_properties(LuauAPI_GeneratedBindings PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(LuauAPI_GeneratedBindings PUBLIC ${LUAUAPI_GEN_DIR}/src)
target_link_libraries(LuauAPI_GeneratedBindings PUBLIC LuauAPI_Runtime)

if(NOT LUAUAPI_STRICT_GENERATED_WARNINGS)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER MATCHES "clang-cl")
        set_source_files_properties(${LUAUAPI_GENERATED_BINDING_SOURCES} PROPERTIES COMPILE_OPTIONS "/w")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        set_source_files_properties(${LUAUAPI_GENERATED_BINDING_SOURCES} PROPERTIES COMPILE_OPTIONS "-w")
    endif()
else()
    message(STATUS "LuauAPI: compiling generated bindings with warnings enabled")
endif()

add_library(${PROJECT_NAME} SHARED
    src/main.cpp
    $<TARGET_OBJECTS:LuauAPI_Runtime>
    $<TARGET_OBJECTS:LuauAPI_GeneratedBindings>
)
target_include_directories(${PROJECT_NAME} PRIVATE include src ${LUAUAPI_GEN_DIR}/src)
target_link_libraries(${PROJECT_NAME}
    Luau.VM
    Luau.Compiler
    Luau.Ast
    Luau.Require
    Luau.CodeGen
    imgui-cocos
    geode-sdk
)
luauapi_apply_compile_settings(${PROJECT_NAME})
setup_geode_mod(${PROJECT_NAME})
endif()

if(LUAUAPI_BUILD_TESTS)
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

    add_executable(luauapi_tests
        src/api.cpp
        src/lua/bindings/Binding.cpp
        src/lua/bindings/framework/BindingHost.cpp
        src/lua/bindings/framework/Fields.cpp
        src/lua/bindings/framework/Usertype.cpp
        src/lua/bindings/framework/UsertypeRegistry.cpp
        src/lua/bindings/framework/LuaDelegate.cpp
        src/lua/bindings/framework/LuaTrampolineRegistry.cpp
        src/lua/bindings/task/TaskBinding.cpp
        src/lua/bindings/geode/CurrentMod.cpp
        src/lua/bindings/geode/GeodeFsBinding.cpp
        src/lua/bindings/geode/GeodeJsonBinding.cpp
        src/lua/bindings/geode/JsonConvert.cpp
        src/lua/bindings/imgui/ImGuiDrawScheduler.cpp
        src/lua/bindings/task/TaskScheduler.cpp
        src/lua/module/BytecodeCacheKey.cpp
        src/lua/module/Requirer.cpp
        src/lua/runtime/Loadstring.cpp
        src/lua/runtime/Runtime.cpp
        tests/allocator_accounting_tests.cpp
        tests/api_tests.cpp
        tests/binding_tests.cpp
        tests/bytecode_cache_accounting_tests.cpp
        tests/current_mod_tests.cpp
        tests/fields_tests.cpp
        tests/bytecode_cache_key_tests.cpp
        tests/callback_tests.cpp
        tests/geode_fs_path_tests.cpp
        tests/imgui_scheduler_tests.cpp
        tests/loadstring_tests.cpp
        tests/path_sandbox_tests.cpp
        tests/path_rules_tests.cpp
        tests/require_path_tests.cpp
        tests/requirer_root_tests.cpp
        tests/resource_bounds_tests.cpp
        tests/runtime_tests.cpp
        tests/task_scheduler_tests.cpp
        tests/usertype_tests.cpp
        tests/vector_view_tests.cpp
        tests/misc_correctness_tests.cpp
    )
    target_compile_features(luauapi_tests PRIVATE cxx_std_23)
    target_include_directories(luauapi_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/host/include
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/host
        src
        include
    )
    target_compile_definitions(luauapi_tests PRIVATE LUAUAPI_HOST_TESTS=1)
    target_link_libraries(luauapi_tests PRIVATE
        Catch2::Catch2WithMain
        fmt::fmt
        Luau.VM
        Luau.Compiler
        Luau.CodeGen
        Luau.Require
    )

    get_target_property(LUAUAPI_TEST_LINK_LIBS luauapi_tests LINK_LIBRARIES)
    if(LUAUAPI_TEST_LINK_LIBS)
        foreach(LUAUAPI_TEST_LINK_LIB IN LISTS LUAUAPI_TEST_LINK_LIBS)
            get_filename_component(LUAUAPI_TEST_LINK_NAME "${LUAUAPI_TEST_LINK_LIB}" NAME)
            foreach(LUAUAPI_FORBIDDEN_TEST_LIB
                GeodeBindings
                libcocos2d.lib
                libExtensions.lib
                glew32.lib
                fmod.lib
                opengl32
                imgui-cocos
                imgui
            )
                if(LUAUAPI_TEST_LINK_LIB STREQUAL LUAUAPI_FORBIDDEN_TEST_LIB OR
                    LUAUAPI_TEST_LINK_NAME STREQUAL LUAUAPI_FORBIDDEN_TEST_LIB)
                    message(FATAL_ERROR "luauapi_tests must not link ${LUAUAPI_FORBIDDEN_TEST_LIB}")
                endif()
            endforeach()
        endforeach()
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
