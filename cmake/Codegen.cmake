macro(luauapi_set_codegen_platform)
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
endmacro()

macro(luauapi_set_gd_version_from_mod_json)
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
endmacro()

macro(luauapi_codegen_cmd OUT_VAR)
    set(${OUT_VAR}
        ${CMAKE_COMMAND} -E env "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/tools"
        ${Python3_EXECUTABLE} -m luau_codegen)
endmacro()

macro(luauapi_codegen_cli_args OUT_VAR MODE)
    set(LUAUAPI_ARGS
        --bindings ${LUAUAPI_BINDINGS_DIR}
        --platform ${LUAUAPI_CODEGEN_PLATFORM}
        --geode-sdk "${LUAUAPI_GEODE_SDK_PATH}")
    if ("${MODE}" STREQUAL "list-all-outputs")
        list(APPEND LUAUAPI_ARGS --list-all-outputs)
    elseif ("${MODE}" STREQUAL "generate")
        list(APPEND LUAUAPI_ARGS
            --out ${LUAUAPI_GEN_DIR}
            --types-out ${LUAUAPI_TYPES_DIR}
            --delegate-specs-out ${LUAUAPI_DELEGATE_SPECS_OUT})
    else()
        message(FATAL_ERROR "Unknown luauapi_codegen_cli_args MODE: ${MODE}")
    endif()
    set(${OUT_VAR} ${LUAUAPI_ARGS})
endmacro()

macro(luauapi_setup_codegen)
    luauapi_set_codegen_platform()
    luauapi_set_gd_version_from_mod_json()

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
    set(LUAUAPI_GEODE_ENUMS_HEADER
        "${geode_bindings_SOURCE_DIR}/bindings/include/Geode/Enums.hpp")
    set(LUAUAPI_GEODE_COCOS_HEADERS
        "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/cocos/base_nodes/CCNode.h"
        "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/cocos/cocoa/CCArray.h"
        "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/cocos/robtop/keyboard_dispatcher/CCKeyboardDelegate.h")
    set(LUAUAPI_GEODE_UTILS_HEADERS
        "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/utils/general.hpp"
        "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/utils/string.hpp"
        "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/utils/random.hpp"
        "${LUAUAPI_GEODE_SDK_PATH}/loader/include/Geode/utils/cocos.hpp")

    luauapi_codegen_cmd(LUAUAPI_CODEGEN_CMD)
    luauapi_codegen_cli_args(LUAUAPI_CODEGEN_ARGS "list-all-outputs")
    execute_process(
        COMMAND ${LUAUAPI_CODEGEN_CMD} ${LUAUAPI_CODEGEN_ARGS}
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

    luauapi_codegen_cli_args(LUAUAPI_CODEGEN_ARGS "generate")

    add_custom_command(
        OUTPUT ${LUAUAPI_CODEGEN_STAMP}
        BYPRODUCTS
                ${LUAUAPI_CODEGEN_BINDING_BYPRODUCTS}
                ${LUAUAPI_GENERATED_TYPE_FILES}
                ${LUAUAPI_GENERATED_METADATA_FILES}
                ${LUAUAPI_DELEGATE_SPECS_OUT}
        COMMAND ${LUAUAPI_CODEGEN_CMD} ${LUAUAPI_CODEGEN_ARGS}
        COMMAND ${CMAKE_COMMAND} -E touch ${LUAUAPI_CODEGEN_STAMP}
        DEPENDS
            ${LUAUAPI_CODEGEN_PY}
            ${LUAUAPI_BRO_FILES}
            ${LUAUAPI_EXTRA_DLUAU_FILES}
            ${LUAUAPI_GEODE_UI_HEADERS}
            ${LUAUAPI_GEODE_UI_MANIFEST}
            ${LUAUAPI_GEODE_ENUMS_HEADER}
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

    list(LENGTH LUAUAPI_GENERATED_BINDING_SOURCES LUAUAPI_GENERATED_BINDING_COUNT)
    message(STATUS "LuauAPI GD bindings version: ${LUAUAPI_GD_VERSION} (platform ${LUAUAPI_CODEGEN_PLATFORM}, ${LUAUAPI_GENERATED_BINDING_COUNT} generated binding sources)")
endmacro()
