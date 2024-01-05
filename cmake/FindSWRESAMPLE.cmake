cmake_minimum_required(VERSION 3.11)
find_package(PkgConfig)
if (PkgConfig_FOUND)
    pkg_check_modules(PC_SWRESAMPLE QUIET IMPORTED_TARGET GLOBAL libswresample)
endif()

if (PC_SWRESAMPLE_FOUND)
    set(SWRESAMPLE_FOUND TRUE)
    set(SWRESAMPLE_VERSION ${PC_SWRESAMPLE_VERSION})
    set(SWRESAMPLE_VERSION_STRING ${PC_SWRESAMPLE_STRING})
    set(SWRESAMPLE_LIBRARYS ${PC_SWRESAMPLE_LIBRARIES})
    if (USE_STATIC_LIBS)
        set(SWRESAMPLE_INCLUDE_DIRS ${PC_SWRESAMPLE_STATIC_INCLUDE_DIRS})
    else()
        set(SWRESAMPLE_INCLUDE_DIRS ${PC_SWRESAMPLE_INCLUDE_DIRS})
    endif()
    if (NOT SWRESAMPLE_INCLUDE_DIRS)
        find_path(SWRESAMPLE_INCLUDE_DIRS NAMES libswresample/swresample.h)
        if (SWRESAMPLE_INCLUDE_DIRS)
            target_include_directories(PkgConfig::PC_SWRESAMPLE INTERFACE ${SWRESAMPLE_INCLUDE_DIRS})
        endif()
    endif()
    if (NOT TARGET SWRESAMPLE::SWRESAMPLE)
        add_library(SWRESAMPLE::SWRESAMPLE ALIAS PkgConfig::PC_SWRESAMPLE)
    endif()
else()
    message(FATAL_ERROR "failed to find libswresample.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SWRESAMPLE
    FOUND_VAR SWRESAMPLE_FOUND
    REQUIRED_VARS
        SWRESAMPLE_LIBRARYS
        SWRESAMPLE_INCLUDE_DIRS
    VERSION_VAR SWRESAMPLE_VERSION
)
