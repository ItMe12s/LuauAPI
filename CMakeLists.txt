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

FetchContent_Declare(
    geode_bindings
    GIT_REPOSITORY https://github.com/geode-sdk/bindings.git
    GIT_TAG        main
    SOURCE_SUBDIR  _fetch_only_
)
FetchContent_MakeAvailable(geode_bindings)

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

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

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
    set(LUAUAPI_CODEGEN_PLATFORM "android")
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
set(LUAUAPI_TYPES_FILES
    "${LUAUAPI_TYPES_DIR}/geode_cocos2d.d.luau"
    "${LUAUAPI_TYPES_DIR}/geode_gd.d.luau"
    "${LUAUAPI_TYPES_DIR}/geode.d.luau"
)

if (NOT EXISTS "${LUAUAPI_BINDINGS_DIR}/GeometryDash.bro")
    message(FATAL_ERROR "LuauAPI bindings dir missing: ${LUAUAPI_BINDINGS_DIR}")
endif()

execute_process(
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/luau_codegen/codegen.py
            --bindings ${LUAUAPI_BINDINGS_DIR}
            --list-outputs
            --platform ${LUAUAPI_CODEGEN_PLATFORM}
    OUTPUT_VARIABLE LUAUAPI_GENERATED_BINDINGS_RAW
    RESULT_VARIABLE LUAUAPI_LIST_OUTPUTS_RESULT
    ERROR_VARIABLE LUAUAPI_LIST_OUTPUTS_ERROR
)
if (NOT LUAUAPI_LIST_OUTPUTS_RESULT EQUAL 0)
    message(FATAL_ERROR "LuauAPI codegen --list-outputs failed: ${LUAUAPI_LIST_OUTPUTS_ERROR}")
endif()
string(REPLACE "\r" "" LUAUAPI_GENERATED_BINDINGS_RAW "${LUAUAPI_GENERATED_BINDINGS_RAW}")
string(STRIP "${LUAUAPI_GENERATED_BINDINGS_RAW}" LUAUAPI_GENERATED_BINDINGS_RAW)
string(REPLACE "\n" ";" LUAUAPI_GENERATED_BINDINGS "${LUAUAPI_GENERATED_BINDINGS_RAW}")
list(FILTER LUAUAPI_GENERATED_BINDINGS EXCLUDE REGEX "^$")
list(TRANSFORM LUAUAPI_GENERATED_BINDINGS PREPEND "${LUAUAPI_GEN_DIR}/")

file(GLOB LUAUAPI_CODEGEN_PY CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/luau_codegen/*.py)
file(GLOB LUAUAPI_BRO_FILES CONFIGURE_DEPENDS
    ${LUAUAPI_BINDINGS_DIR}/*.bro)

add_custom_command(
    OUTPUT ${LUAUAPI_GENERATED_BINDINGS} ${LUAUAPI_TYPES_FILES}
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/luau_codegen/codegen.py
            --bindings ${LUAUAPI_BINDINGS_DIR}
            --out ${LUAUAPI_GEN_DIR}
            --types-out ${LUAUAPI_TYPES_DIR}
            --platform ${LUAUAPI_CODEGEN_PLATFORM}
    DEPENDS ${LUAUAPI_CODEGEN_PY} ${LUAUAPI_BRO_FILES}
    COMMENT "LuauAPI Broma codegen"
    VERBATIM
)
add_custom_target(luauapi_codegen DEPENDS ${LUAUAPI_GENERATED_BINDINGS} ${LUAUAPI_TYPES_FILES})

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
set(LUAUAPI_GENERATED_BINDING_SOURCES ${LUAUAPI_GENERATED_BINDINGS})
list(FILTER LUAUAPI_GENERATED_BINDING_SOURCES INCLUDE REGEX "\\.cpp$")
list(APPEND SOURCES ${LUAUAPI_GENERATED_BINDING_SOURCES})
set_source_files_properties(${LUAUAPI_GENERATED_BINDING_SOURCES} PROPERTIES GENERATED TRUE)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
add_dependencies(${PROJECT_NAME} luauapi_codegen)

setup_geode_mod(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_include_directories(${PROJECT_NAME} PRIVATE include src ${LUAUAPI_GEN_DIR}/src)
target_link_libraries(${PROJECT_NAME} Luau.VM Luau.Compiler Luau.Ast Luau.Require Luau.CodeGen)
target_compile_definitions(${PROJECT_NAME} PRIVATE LUAUAPI_EXPORTING=1)

# if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER MATCHES "clang-cl")
#     target_compile_options(${PROJECT_NAME} PRIVATE /W4)
# elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
#     target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
# endif()
# if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#     target_compile_options(${PROJECT_NAME} PRIVATE -Wno-gnu-zero-variadic-macro-arguments)
# endif()

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
    target_include_directories(luauapi_tests PRIVATE src)
    target_link_libraries(luauapi_tests PRIVATE Catch2::Catch2WithMain geode-sdk)

    if(WIN32)
        set(LUAUAPI_TEST_GEODE_VERSION "${GEODE_VERSION}")
        if("${LUAUAPI_TEST_GEODE_VERSION}" STREQUAL "")
            file(READ "$ENV{GEODE_SDK}/VERSION" LUAUAPI_TEST_GEODE_VERSION)
            string(STRIP "${LUAUAPI_TEST_GEODE_VERSION}" LUAUAPI_TEST_GEODE_VERSION)
        endif()

        set(LUAUAPI_TEST_GEODE_RUNTIME_DIR "$ENV{GEODE_SDK}/bin/${LUAUAPI_TEST_GEODE_VERSION}")
        if(NOT EXISTS "${LUAUAPI_TEST_GEODE_RUNTIME_DIR}/Geode.dll")
            set(LUAUAPI_TEST_GEODE_RUNTIME_DIR "$ENV{GEODE_SDK}/bin/nightly")
        endif()
        if(NOT EXISTS "${LUAUAPI_TEST_GEODE_RUNTIME_DIR}/Geode.dll")
            message(FATAL_ERROR "Geode runtime DLL missing: ${LUAUAPI_TEST_GEODE_RUNTIME_DIR}/Geode.dll")
        endif()

        file(GLOB LUAUAPI_TEST_GEODE_RUNTIME_DLLS CONFIGURE_DEPENDS
            "${LUAUAPI_TEST_GEODE_RUNTIME_DIR}/*.dll")
        if(NOT LUAUAPI_TEST_GEODE_RUNTIME_DLLS)
            message(FATAL_ERROR "No Geode runtime DLLs found in: ${LUAUAPI_TEST_GEODE_RUNTIME_DIR}")
        endif()

        add_custom_command(
            TARGET luauapi_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${LUAUAPI_TEST_GEODE_RUNTIME_DLLS}
                    $<TARGET_FILE_DIR:luauapi_tests>
            COMMAND_EXPAND_LISTS
            VERBATIM
        )
    endif()

    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
    include(Catch)
    catch_discover_tests(luauapi_tests DISCOVERY_MODE PRE_TEST)
endif()
