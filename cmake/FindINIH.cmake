cmake_minimum_required(VERSION 3.11)
find_package(PkgConfig)
if (PkgConfig_FOUND)
    pkg_check_modules(PC_INIH QUIET IMPORTED_TARGET GLOBAL inih)
endif()

if (PC_INIH_FOUND)
    set(INIH_FOUND TRUE)
    set(INIH_VERSION ${PC_INIH_VERSION})
    set(INIH_VERSION_STRING ${PC_INIH_STRING})
    set(INIH_LIBRARYS ${PC_INIH_LIBRARIES})
    if (USE_STATIC_LIBS)
        set(INIH_INCLUDE_DIRS ${PC_INIH_STATIC_INCLUDE_DIRS})
    else()
        set(INIH_INCLUDE_DIRS ${PC_INIH_INCLUDE_DIRS})
    endif()
    if (NOT INIH_INCLUDE_DIRS)
        find_path(INIH_INCLUDE_DIRS NAMES ini.h)
        if (INIH_INCLUDE_DIRS)
            target_include_directories(PkgConfig::PC_INIH INTERFACE ${INIH_INCLUDE_DIRS})
        endif()
    endif()
    if (NOT TARGET INIH::INIH)
        add_library(INIH::INIH ALIAS PkgConfig::PC_INIH)
    endif()
else()
    message(FATAL_ERROR "failed to find inih. Use -DENABLE_INIH=OFF to disable ini support.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(INIH
    FOUND_VAR INIH_FOUND
    REQUIRED_VARS
        INIH_LIBRARYS
        INIH_INCLUDE_DIRS
    VERSION_VAR INIH_VERSION
)
