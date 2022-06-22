cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(vld
        VERSION 2.5.2
        DESCRIPTION "Visual Leak Detector"
        HOMEPAGE_URL "https://github.com/btolfa/vld"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(vld SHARED)

if (CMAKE_VS_PLATFORM_NAME STREQUAL "Win32")
    set_target_properties(vld
        PROPERTIES SUFFIX "_x86.dll"
                   EXPORT_NAME "vld_x86.lib"
                   LIBRARY_OUTPUT_NAME "vld_x86.lib"
                   #COMPILE_PDB_NAME "vld_x86"
    )
endif()

if (CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
    set_target_properties(vld
        PROPERTIES SUFFIX "_x64.dll"
                   EXPORT_NAME "vld__x64.lib"
                   LIBRARY_OUTPUT_NAME "vld_x64.lib"
                   #COMPILE_PDB_NAME "vld_x64"
    )
endif()

target_include_directories(vld PRIVATE setup)
target_compile_definitions(vld PRIVATE UNICODE _UNICODE)

add_subdirectory(lib/cppformat fmt)
target_link_libraries(vld PRIVATE fmt)

target_sources(vld PRIVATE
    src/callstack.cpp
    src/dllspatches.cpp
    src/ntapi.cpp
    src/stdafx.cpp
    src/utility.cpp
    src/vld.cpp
    src/vldapi.cpp
    src/vldheap.cpp
    src/vld_hooks.cpp
    src/callstack.h
    src/criticalsection.h
    src/crtmfcpatch.h
    src/dbghelp.h
    src/map.h
    src/ntapi.h
    src/resource.h
    src/set.h
    setup/version.h
    src/stdafx.h
    src/tree.h
    src/utility.h
    src/vld.h
    src/vldallocator.h
    src/vldheap.h
    src/vldint.h
    src/vld_def.h
    src/vld.natvis
    src/vld.rc
)

if (CMAKE_VS_PLATFORM_NAME STREQUAL "Win32")
    target_sources(vld PRIVATE src/vld.dll.dependency.x86.manifest)
endif()

if (CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
    target_sources(vld PRIVATE src/vld.dll.dependency.x64.manifest)
endif()

target_include_directories(vld INTERFACE src)

target_compile_options(vld PRIVATE $<$<CONFIG:Release>:/Ot /GT /GF /Gy->)

enable_testing()

add_subdirectory(lib/gtest gtest)
add_subdirectory(src/tests tests) 