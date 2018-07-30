#
# Source: https://github.com/obsproject/obs-browser/blob/89aa4ee8b6c7ae12acbd90158feca1fe61dedf7d/FindCEF.cmake
#
include(FindPackageHandleStandardArgs)

SET(CEF_ROOT_DIR "/opt/cef" CACHE PATH "Path to a CEF distributed build")
message(STATUS "Looking for Chromium Embedded Framework in ${CEF_ROOT_DIR}")

find_path(CEF_INCLUDE_DIR
    "cef_version.h"
    NAMES
    cef_version.h
    HINTS
    ${CEF_ROOT_DIR}
    PATH_SUFFIXES
    include/
)

find_library(CEF_LIBRARY
    "Chromium Embedded Framework"
    NAMES
    cef
    libcef
    PATHS
    ${CEF_ROOT_DIR}
    ${CEF_ROOT_DIR}
    PATH_SUFFIXES
    /Release
)
find_library(CEF_LIBRARY_DEBUG
    "Chromium Embedded Framework"
    NAMES
    cef
    libcef
    PATHS
    ${CEF_ROOT_DIR}
    ${CEF_ROOT_DIR}
    PATH_SUFFIXES
    /Debug
)

find_library(CEFWRAPPER_LIBRARY
    "CEF wrapper"
    NAMES
    cef_dll_wrapper
    libcef_dll_wrapper
    PATHS
    ${CEF_ROOT_DIR}
    ${CEF_ROOT_DIR}/libcef_dll
    ${CEF_ROOT_DIR}/libcef_dll_wrapper
    ${CEF_ROOT_DIR}/build/libcef_dll
    ${CEF_ROOT_DIR}/build/libcef_dll_wrapper
    PATH_SUFFIXES
    /Release
)

find_library(CEFWRAPPER_LIBRARY_DEBUG
    "CEF wrapper"
    NAMES
    cef_dll_wrapper
    libcef_dll_wrapper
    PATHS
    ${CEF_ROOT_DIR}/libcef_dll/Debug
    ${CEF_ROOT_DIR}/libcef_dll_wrapper/Debug
    ${CEF_ROOT_DIR}/build/libcef_dll/Debug
    ${CEF_ROOT_DIR}/build/libcef_dll_wrapper/Debug
)

find_path(CEF_RESOURCE_DIR
    "CEF resources directory"
    NAMES
    cef_100_percent.pak
    cef_200_percent.pak
    cef_extensions.pak
    cef.pak
    devtools_resources.pak
    icudtl.dat
    PATHS
    ${CEF_ROOT_DIR}
    PATH_SUFFIXES
    /Resources
)

if(NOT CEF_LIBRARY)
    message(WARNING "Could not find the CEF shared library" )
    set(CEF_FOUND false)
    return()
endif()

get_filename_component(CEF_LIBRARY_PATH ${CEF_LIBRARY} PATH)

if(NOT CEFWRAPPER_LIBRARY)
    message(WARNING "Could not find the CEF wrapper library" )
    set(CEF_FOUND false)
    return()
endif()

set(CEF_LIBRARIES
    ${CEFWRAPPER_LIBRARY}
    ${CEF_LIBRARY})

set(CEF_INCLUDE_DIRS
    ${CEF_INCLUDE_DIR}
)

if (CEF_LIBRARY_DEBUG AND CEFWRAPPER_LIBRARY_DEBUG)
    list(APPEND CEF_LIBRARIES
        ${CEFWRAPPER_LIBRARY_DEBUG}
        ${CEF_LIBRARY_DEBUG})
endif()


find_package_handle_standard_args(CEF
    DEFAULT_MSG
    CEF_LIBRARY
    CEFWRAPPER_LIBRARY
    CEF_RESOURCE_DIR
    CEF_INCLUDE_DIR
)

set(CEF_INCLUDE_DIR
    "${CEF_INCLUDE_DIR}"
    "${CEF_INCLUDE_DIR}/.."
)

mark_as_advanced(
    CEF_LIBRARY
    CEFWRAPPER_LIBRARY
    CEF_LIBRARIES
    CEF_INCLUDE_DIR
    CEF_RESOURCE_DIR
)
