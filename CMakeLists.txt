cmake_minimum_required(VERSION 3.16)
project(mempatch C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# -----------------------------------------------------------------------------
# Lua 5.1 (vendored from CryEngine 5.2.3 SDK — same Lua the game ships)
# -----------------------------------------------------------------------------
file(GLOB LUA_SOURCES vendor/lua/*.c)
add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC vendor/lua)
if(MSVC)
    target_compile_options(lua PRIVATE /wd4244 /wd4267 /wd4334 /wd4310)
    target_compile_definitions(lua PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# -----------------------------------------------------------------------------
# MinHook (vendored)
# -----------------------------------------------------------------------------
add_library(minhook STATIC
    vendor/minhook/src/buffer.c
    vendor/minhook/src/hook.c
    vendor/minhook/src/trampoline.c
    vendor/minhook/src/hde/hde64.c
)
target_include_directories(minhook PUBLIC vendor/minhook/include)
target_include_directories(minhook PRIVATE vendor/minhook/src)

# -----------------------------------------------------------------------------
# mempatch.asi
# -----------------------------------------------------------------------------
add_library(mempatch SHARED
    src/dllmain.cpp
    src/log.cpp
    src/pe_helpers.cpp
    src/patch_engine.cpp
    src/config.cpp
    src/hooks.cpp
    src/lua_bind.cpp
)

target_include_directories(mempatch PRIVATE
    src
    vendor/lua
    vendor/minhook/include
    vendor/tomlplusplus
)

target_link_libraries(mempatch PRIVATE lua minhook)

if(MSVC)
    target_compile_definitions(mempatch PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    target_compile_options(mempatch PRIVATE /W3 /MT)
    set_property(TARGET mempatch PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    set_property(TARGET lua      PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
    set_property(TARGET minhook  PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
else()
    # MinGW cross-compile
    target_compile_definitions(mempatch PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    target_compile_options(mempatch PRIVATE -Wall -Wno-unknown-pragmas)
    target_link_options(mempatch PRIVATE -static -static-libgcc -static-libstdc++)
endif()

# Output as <name>.asi instead of <name>.dll
set_target_properties(mempatch PROPERTIES
    PREFIX ""
    SUFFIX ".asi"
    OUTPUT_NAME "mempatch"
)
