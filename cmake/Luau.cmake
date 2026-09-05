set(LUAU_BUILD_CLI   OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_WEB   OFF CACHE BOOL "" FORCE)
set(LUAU_EXTERN_C    OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    luau
    GIT_REPOSITORY https://github.com/luau-lang/luau.git
    GIT_TAG        0.737
)
FetchContent_MakeAvailable(luau)

set(LUAUAPI_LUAU_TARGETS
    Luau.Common Luau.Ast Luau.Bytecode Luau.Compiler
    Luau.Config Luau.Analysis Luau.CodeGen Luau.VM Luau.Require)
set(LUAUAPI_LUAU_LINK_LIBS
    Luau.VM Luau.Compiler Luau.Ast Luau.Require Luau.CodeGen)

foreach(LUAU_TARGET IN LISTS LUAUAPI_LUAU_TARGETS)
    if (TARGET ${LUAU_TARGET})
        get_target_property(LUAU_PUBLIC_INCLUDES ${LUAU_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
        if (LUAU_PUBLIC_INCLUDES)
            set_target_properties(${LUAU_TARGET} PROPERTIES
                INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${LUAU_PUBLIC_INCLUDES}"
            )
        endif()
    endif()
endforeach()

luauapi_force_msvc_idl0(${LUAUAPI_LUAU_TARGETS} isocline)

# Room for reserved + generated usertype tags (UserdataTags.hpp).
# Long jump for exceptionless Luau.
set(LUAUAPI_EXCEPTIONLESS_TARGETS Luau.VM Luau.CodeGen)
foreach(LUAU_TARGET IN LISTS LUAUAPI_EXCEPTIONLESS_TARGETS)
    if (TARGET ${LUAU_TARGET})
        target_compile_definitions(${LUAU_TARGET} PUBLIC
            LUA_UTAG_LIMIT=2048
            LUA_USE_LONGJMP=1
        )
        target_compile_options(${LUAU_TARGET} PRIVATE
            $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fno-exceptions>
            $<$<CXX_COMPILER_ID:MSVC>:/EH->
        )
        target_compile_definitions(${LUAU_TARGET} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>:_HAS_EXCEPTIONS=0>
        )
    endif()
endforeach()
