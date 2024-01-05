cmake_minimum_required(VERSION 3.11)
find_package(PkgConfig)
if (PkgConfig_FOUND)
    pkg_check_modules(PC_CDDB QUIET IMPORTED_TARGET GLOBAL libcddb)
endif()

if (PC_CDDB_FOUND)
    set(CDDB_FOUND TRUE)
    set(CDDB_VERSION ${PC_CDDB_VERSION})
    set(CDDB_VERSION_STRING ${PC_CDDB_STRING})
    set(CDDB_LIBRARYS ${PC_CDDB_LIBRARIES})
    if (USE_STATIC_LIBS)
        set(CDDB_INCLUDE_DIRS ${PC_CDDB_STATIC_INCLUDE_DIRS})
    else()
        set(CDDB_INCLUDE_DIRS ${PC_CDDB_INCLUDE_DIRS})
    endif()
    if (NOT CDDB_INCLUDE_DIRS)
        find_path(CDDB_INCLUDE_DIRS NAMES cddb/cddb.h)
        if (CDDB_INCLUDE_DIRS)
            target_include_directories(PkgConfig::PC_CDDB INTERFACE ${CDDB_INCLUDE_DIRS})
        endif()
    endif()
    if (NOT TARGET CDDB::CDDB)
        add_library(CDDB::CDDB ALIAS PkgConfig::PC_CDDB)
    endif()
else()
    message(FATAL_ERROR "failed to find libcddb.")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CDDB
    FOUND_VAR CDDB_FOUND
    REQUIRED_VARS
        CDDB_LIBRARYS
        CDDB_INCLUDE_DIRS
    VERSION_VAR CDDB_VERSION
)
