set(OBS_ROOT_DIR
    "/usr"
    CACHE
    PATH "OBS installation prefix"
)

message(STATUS "Looking for OBS Studio in prefix ${OBS_ROOT_DIR}")

if (CMAKE_SIZEOF_VOID_P MATCHES "8")
    set(_LIBSUFFIXES /lib64 /lib)
else()
    set(_LIBSUFFIXES /lib)
endif()

set(_INCSUFFIXES /include)

find_library(OBS_LIBRARY
    NAMES
    obs
    "OBS Studio"
    PATHS
    "${OBS_ROOT_DIR}"
    PATH_SUFFIXES
    "${_LIBSUFFIXES}"
)

find_path(OBS_INCLUDE_DIR
    NAMES
    obs.h obs.hpp
    "OBS Studio"
    PATHS
    "${OBS_ROOT_DIR}"
    "/usr"
    PATH_SUFFIXES
    include/
    include/obs
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OBS
    DEFAULT_MSG
    OBS_LIBRARY
    OBS_INCLUDE_DIR
    ${_deps_check}
)

if (OBS_FOUND)
    set(OBS_LIBRARIES "${OBS_LIBRARY}")
    mark_as_advanced(OBS_ROOT_DIR)
endif()

mark_as_advanced(
    OBS_INCLUDE_DIR
    OBS_LIBRARY
)
