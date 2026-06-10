set(LUAUAPI_MBEDTLS_GIT_TAG "v3.6.6" CACHE STRING "Pinned mbedtls tag")
set(LUAUAPI_IXWEBSOCKET_GIT_TAG "v12.0.0" CACHE STRING "Pinned IXWebSocket tag")

set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    mbedtls
    GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
    GIT_TAG        ${LUAUAPI_MBEDTLS_GIT_TAG}
)

if(WIN32
    AND CMAKE_C_COMPILER_ID STREQUAL "Clang"
    AND CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
    set(LUAUAPI_SAVED_C_SIMULATE_ID "${CMAKE_C_SIMULATE_ID}")
    set(CMAKE_C_SIMULATE_ID "")
endif()
FetchContent_MakeAvailable(mbedtls)
if(DEFINED LUAUAPI_SAVED_C_SIMULATE_ID)
    set(CMAKE_C_SIMULATE_ID "${LUAUAPI_SAVED_C_SIMULATE_ID}")
    unset(LUAUAPI_SAVED_C_SIMULATE_ID)
endif()
message(STATUS "LuauAPI mbedtls: ${LUAUAPI_MBEDTLS_GIT_TAG}")

set(MBEDTLS_FOUND TRUE)
set(MBEDTLS_INCLUDE_DIRS ${mbedtls_SOURCE_DIR}/include)
set(MBEDTLS_LIBRARIES mbedtls mbedx509 mbedcrypto)
set(MBEDTLS_VERSION_GREATER_THAN_3 TRUE)

set(USE_TLS TRUE CACHE BOOL "" FORCE)
set(USE_MBED_TLS TRUE CACHE BOOL "" FORCE)
set(USE_ZLIB FALSE CACHE BOOL "" FORCE)
set(USE_WS FALSE CACHE BOOL "" FORCE)
set(USE_TEST FALSE CACHE BOOL "" FORCE)
set(IXWEBSOCKET_INSTALL FALSE CACHE BOOL "" FORCE)
FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG        ${LUAUAPI_IXWEBSOCKET_GIT_TAG}
)
FetchContent_MakeAvailable(ixwebsocket)
message(STATUS "LuauAPI IXWebSocket: ${LUAUAPI_IXWEBSOCKET_GIT_TAG}")

if(TARGET ixwebsocket AND MBEDTLS_VERSION_GREATER_THAN_3)
    target_compile_definitions(ixwebsocket PRIVATE IXWEBSOCKET_USE_MBED_TLS_MIN_VERSION_3)
endif()

luauapi_apply_windows_socket_bootstrap(ixwebsocket)
luauapi_force_msvc_idl0(ixwebsocket mbedtls mbedx509 mbedcrypto everest p256m builtin)

foreach(WS_TARGET IN ITEMS ixwebsocket mbedtls mbedx509 mbedcrypto everest p256m builtin)
    if (TARGET ${WS_TARGET})
        set_target_properties(${WS_TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
        if (WIN32)
            target_compile_definitions(${WS_TARGET} PRIVATE WIN32_LEAN_AND_MEAN)
        endif()
    endif()
endforeach()
