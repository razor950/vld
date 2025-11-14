# FindDetours.cmake - Locate Microsoft Detours library
#
# This module defines:
#  Detours_FOUND - System has Detours
#  Detours_INCLUDE_DIRS - Detours include directories
#  Detours_LIBRARIES - Libraries needed to use Detours
#  Detours::Detours - Imported target for Detours
#
# You can set these variables to help locate Detours:
#  DETOURS_ROOT - Root directory of Detours installation
#  DETOURS_INCLUDE_DIR - Directory containing detours.h
#  DETOURS_LIBRARY - Path to detours.lib

# Try to find using vcpkg first
find_package(Detours CONFIG QUIET)
if(Detours_FOUND)
    message(STATUS "Found Detours via CONFIG")
    return()
endif()

# Otherwise, use manual search
find_path(DETOURS_INCLUDE_DIR
    NAMES detours.h
    HINTS
        ${DETOURS_ROOT}/include
        ${CMAKE_SOURCE_DIR}/external/Detours/include
        ${CMAKE_SOURCE_DIR}/lib/Detours/include
        $ENV{DETOURS_ROOT}/include
    PATH_SUFFIXES
        detours
)

# Determine library name based on architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(DETOURS_LIB_SUFFIX "X64")
else()
    set(DETOURS_LIB_SUFFIX "X86")
endif()

find_library(DETOURS_LIBRARY
    NAMES detours
    HINTS
        ${DETOURS_ROOT}/lib
        ${CMAKE_SOURCE_DIR}/external/Detours/lib/${DETOURS_LIB_SUFFIX}
        ${CMAKE_SOURCE_DIR}/lib/Detours/lib
        $ENV{DETOURS_ROOT}/lib
    PATH_SUFFIXES
        ${DETOURS_LIB_SUFFIX}
        lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Detours
    REQUIRED_VARS
        DETOURS_LIBRARY
        DETOURS_INCLUDE_DIR
)

if(Detours_FOUND AND NOT TARGET Detours::Detours)
    add_library(Detours::Detours UNKNOWN IMPORTED)
    set_target_properties(Detours::Detours PROPERTIES
        IMPORTED_LOCATION "${DETOURS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${DETOURS_INCLUDE_DIR}"
    )

    set(Detours_LIBRARIES ${DETOURS_LIBRARY})
    set(Detours_INCLUDE_DIRS ${DETOURS_INCLUDE_DIR})

    mark_as_advanced(DETOURS_INCLUDE_DIR DETOURS_LIBRARY)
endif()
