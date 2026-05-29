cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(LuauAPI VERSION 1.0.0)

include(FetchContent)
include(CTest)

option(LUAUAPI_BUILD_TESTS "Build LuauAPI host unit tests" OFF)

set(LUAU_BUILD_CLI   OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_WEB   OFF CACHE BOOL "" FORCE)
set(LUAU_EXTERN_C    OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    luau
    GIT_REPOSITORY https://github.com/luau-lang/luau.git
    GIT_TAG        0.722
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
        target_compile_definitions(${LUAU_TARGET} PUBLIC LUA_UTAG_LIMIT=1024)
    endif()
endforeach()

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)

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
set(LUAUAPI_CODEGEN_STAMP "${LUAUAPI_GEN_DIR}/codegen.stamp")
file(TO_CMAKE_PATH "$ENV{GEODE_SDK}" LUAUAPI_GEODE_SDK_PATH)

if (NOT EXISTS "${LUAUAPI_BINDINGS_DIR}/GeometryDash.bro")
    message(FATAL_ERROR "LuauAPI bindings dir missing: ${LUAUAPI_BINDINGS_DIR}")
endif()

file(GLOB LUAUAPI_CODEGEN_PY CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/luau_codegen/*.py)
file(GLOB LUAUAPI_BRO_FILES CONFIGURE_DEPENDS
    ${LUAUAPI_BINDINGS_DIR}/*.bro)
file(GLOB LUAUAPI_EXTRA_DLUAU_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/luau_codegen/extra_bindings/*.dluau)
file(GLOB LUAUAPI_GEODE_UI_HEADERS CONFIGURE_DEPENDS
    "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/ui/*.hpp")

execute_process(
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/luau_codegen/codegen.py
            --bindings ${LUAUAPI_BINDINGS_DIR}
            --list-outputs
            --platform ${LUAUAPI_CODEGEN_PLATFORM}
            --geode-sdk "${LUAUAPI_GEODE_SDK_PATH}"
    RESULT_VARIABLE LUAUAPI_LIST_OUTPUTS_RESULT
    OUTPUT_VARIABLE LUAUAPI_LIST_OUTPUTS_STDOUT
    ERROR_VARIABLE LUAUAPI_LIST_OUTPUTS_STDERR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if (NOT LUAUAPI_LIST_OUTPUTS_RESULT EQUAL 0)
    message(FATAL_ERROR "LuauAPI codegen output listing failed: ${LUAUAPI_LIST_OUTPUTS_STDERR}")
endif()
set(LUAUAPI_CODEGEN_BINDING_BYPRODUCTS)
if (LUAUAPI_LIST_OUTPUTS_STDOUT)
    string(REPLACE "\n" ";" LUAUAPI_GENERATED_BINDING_RELS "${LUAUAPI_LIST_OUTPUTS_STDOUT}")
    foreach(LUAUAPI_GENERATED_BINDING_REL IN LISTS LUAUAPI_GENERATED_BINDING_RELS)
        if (LUAUAPI_GENERATED_BINDING_REL)
            list(APPEND LUAUAPI_CODEGEN_BINDING_BYPRODUCTS
                "${LUAUAPI_GEN_DIR}/${LUAUAPI_GENERATED_BINDING_REL}")
        endif()
    endforeach()
endif()

execute_process(
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/luau_codegen/codegen.py
            --bindings ${LUAUAPI_BINDINGS_DIR}
            --list-type-outputs
            --platform ${LUAUAPI_CODEGEN_PLATFORM}
            --geode-sdk "${LUAUAPI_GEODE_SDK_PATH}"
    RESULT_VARIABLE LUAUAPI_LIST_TYPE_OUTPUTS_RESULT
    OUTPUT_VARIABLE LUAUAPI_LIST_TYPE_OUTPUTS_STDOUT
    ERROR_VARIABLE LUAUAPI_LIST_TYPE_OUTPUTS_STDERR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if (NOT LUAUAPI_LIST_TYPE_OUTPUTS_RESULT EQUAL 0)
    message(FATAL_ERROR "LuauAPI type output listing failed: ${LUAUAPI_LIST_TYPE_OUTPUTS_STDERR}")
endif()
set(LUAUAPI_GENERATED_TYPE_FILES)
if (LUAUAPI_LIST_TYPE_OUTPUTS_STDOUT)
    string(REPLACE "\n" ";" LUAUAPI_GENERATED_TYPE_RELS "${LUAUAPI_LIST_TYPE_OUTPUTS_STDOUT}")
    foreach(LUAUAPI_GENERATED_TYPE_REL IN LISTS LUAUAPI_GENERATED_TYPE_RELS)
        if (LUAUAPI_GENERATED_TYPE_REL)
            list(APPEND LUAUAPI_GENERATED_TYPE_FILES
                "${LUAUAPI_TYPES_DIR}/${LUAUAPI_GENERATED_TYPE_REL}")
        endif()
    endforeach()
endif()

set(LUAUAPI_GENERATED_METADATA_FILES
    "${LUAUAPI_GEN_DIR}/schema.json"
    "${LUAUAPI_GEN_DIR}/report.md"
    "${LUAUAPI_GEN_DIR}/parity.json")

add_custom_command(
    OUTPUT ${LUAUAPI_CODEGEN_STAMP}
    BYPRODUCTS
            ${LUAUAPI_CODEGEN_BINDING_BYPRODUCTS}
            ${LUAUAPI_GENERATED_TYPE_FILES}
            ${LUAUAPI_GENERATED_METADATA_FILES}
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/luau_codegen/codegen.py
            --bindings ${LUAUAPI_BINDINGS_DIR}
            --out ${LUAUAPI_GEN_DIR}
            --types-out ${LUAUAPI_TYPES_DIR}
            --platform ${LUAUAPI_CODEGEN_PLATFORM}
            --geode-sdk "${LUAUAPI_GEODE_SDK_PATH}"
    COMMAND ${CMAKE_COMMAND} -E touch ${LUAUAPI_CODEGEN_STAMP}
    DEPENDS ${LUAUAPI_CODEGEN_PY} ${LUAUAPI_BRO_FILES} ${LUAUAPI_EXTRA_DLUAU_FILES} ${LUAUAPI_GEODE_UI_HEADERS}
    COMMENT "LuauAPI Broma codegen"
    VERBATIM
)
add_custom_target(luauapi_codegen
    DEPENDS
        ${LUAUAPI_CODEGEN_STAMP}
        ${LUAUAPI_CODEGEN_BINDING_BYPRODUCTS}
        ${LUAUAPI_GENERATED_TYPE_FILES}
        ${LUAUAPI_GENERATED_METADATA_FILES})

file(GLOB LUAUAPI_GENERATED_BINDING_SOURCES CONFIGURE_DEPENDS
    "${LUAUAPI_GEN_DIR}/src/bindings_*.cpp")
file(GLOB_RECURSE LUAUAPI_HANDWRITTEN_SOURCES CONFIGURE_DEPENDS src/*.cpp)
set(SOURCES ${LUAUAPI_HANDWRITTEN_SOURCES})
list(APPEND SOURCES ${LUAUAPI_GENERATED_BINDING_SOURCES})
set_source_files_properties(${LUAUAPI_GENERATED_BINDING_SOURCES} PROPERTIES GENERATED TRUE)
message(STATUS "LuauAPI GD bindings version: ${LUAUAPI_GD_VERSION} (platform ${LUAUAPI_CODEGEN_PLATFORM})")

add_library(${PROJECT_NAME} SHARED ${SOURCES})
add_dependencies(${PROJECT_NAME} luauapi_codegen)

setup_geode_mod(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_include_directories(${PROJECT_NAME} PRIVATE include src ${LUAUAPI_GEN_DIR}/src)
target_link_libraries(${PROJECT_NAME} Luau.VM Luau.Compiler Luau.Ast Luau.Require Luau.CodeGen)
target_compile_definitions(${PROJECT_NAME} PRIVATE LUAUAPI_EXPORTING=1)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER MATCHES "clang-cl")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wno-dollar-in-identifier-extension)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-gnu-zero-variadic-macro-arguments)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER MATCHES "clang-cl")
    set_source_files_properties(${LUAUAPI_GENERATED_BINDING_SOURCES} PROPERTIES COMPILE_OPTIONS "/w")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set_source_files_properties(${LUAUAPI_GENERATED_BINDING_SOURCES} PROPERTIES COMPILE_OPTIONS "-w")
endif()

if(WIN32)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER MATCHES "clang-cl")
        target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE -Wa,-mbig-obj)
    endif()
endif()

if(LUAUAPI_BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.15.0
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(luauapi_tests
        tests/allocator_accounting_tests.cpp
        tests/bytecode_cache_key_tests.cpp
        tests/path_sandbox_tests.cpp
        tests/path_rules_tests.cpp
        tests/require_path_tests.cpp
    )
    target_compile_features(luauapi_tests PRIVATE cxx_std_23)
    target_include_directories(luauapi_tests PRIVATE src include)
    target_compile_definitions(luauapi_tests PRIVATE LUAUAPI_HOST_TESTS=1)
    target_link_libraries(luauapi_tests PRIVATE
        Catch2::Catch2WithMain
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
        NAME luauapi_codegen_hooks
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/luau_codegen_hook_tests.py
    )
endif()
