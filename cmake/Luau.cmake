set(LUAU_BUILD_CLI   OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_WEB   OFF CACHE BOOL "" FORCE)
set(LUAU_EXTERN_C    OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    luau
    GIT_REPOSITORY https://github.com/luau-lang/luau.git
    GIT_TAG        0.736
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
if (TARGET Luau.VM)
    target_compile_definitions(Luau.VM PUBLIC LUA_UTAG_LIMIT=2048)
    target_compile_definitions(Luau.VM PUBLIC LUA_USE_LONGJMP=1)
endif()
if (TARGET Luau.CodeGen)
    target_compile_definitions(Luau.CodeGen PUBLIC LUA_UTAG_LIMIT=2048)
    target_compile_definitions(Luau.CodeGen PUBLIC LUA_USE_LONGJMP=1)
endif()

# GeodeTaskHandleBinding.hpp uses try/catch, Catch2 tests need them.
if (TARGET Luau.VM)
    target_compile_options(Luau.VM PRIVATE
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fno-exceptions>
        $<$<CXX_COMPILER_ID:MSVC>:/EH->
    )
    target_compile_definitions(Luau.VM PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:_HAS_EXCEPTIONS=0>
    )
endif()
if (TARGET Luau.CodeGen)
    target_compile_options(Luau.CodeGen PRIVATE
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fno-exceptions>
        $<$<CXX_COMPILER_ID:MSVC>:/EH->
    )
    target_compile_definitions(Luau.CodeGen PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:_HAS_EXCEPTIONS=0>
    )
endif()
