cmake_minimum_required(VERSION 3.17)

project(cd2flac)

if (MSVC)
    add_compile_options(/utf-8)
endif()

include(CheckIncludeFiles)
include(CheckSymbolExists)
include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(AVFORMAT 60 REQUIRED)
find_package(AVCODEC 60 REQUIRED)
find_package(AVDEVICE 60 REQUIRED)
find_package(AVUTIL 58 REQUIRED)
find_package(SWRESAMPLE 4 REQUIRED)
find_package(CDDB REQUIRED)

if (WIN32)
    check_symbol_exists(printf_s "stdio.h" HAVE_PRINTF_S)
    check_symbol_exists(sscanf_s "stdio.h" HAVE_SSCANF_S)
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h")

CHECK_INCLUDE_FILES(getopt.h HAVE_GETOPT_H)
if (NOT HAVE_GETOPT_H)
    add_subdirectory(getopt)
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/getopt")
endif()

set(ENABLE_ICONV OFF CACHE BOOL "Libiconv is not needed.")
add_subdirectory(utils)
include_directories(utils)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_custom_target(cd2flac_version
    ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/src/VERSION.cmake"
)

if (WIN32)
    set(CD2FLAC_RC "${CMAKE_CURRENT_BINARY_DIR}/cd2flac.rc")
    set_source_files_properties(${CD2FLAC_RC} PROPERTIES GENERATED TRUE)
endif()

add_executable(cd2flac
src/main.cpp src/info.h src/info.c src/core.h src/core.cpp src/open.h src/open.c
${CD2FLAC_RC})
add_dependencies(cd2flac cd2flac_version)
target_compile_definitions(cd2flac PRIVATE -DHAVE_CONFIG_H)
if (TARGET getopt)
    target_link_libraries(cd2flac getopt)
endif()
target_link_libraries(cd2flac AVUTIL::AVUTIL AVFORMAT::AVFORMAT AVCODEC::AVCODEC AVDEVICE::AVDEVICE SWRESAMPLE::SWRESAMPLE CDDB::CDDB)
target_link_libraries(cd2flac utils)

install(TARGETS cd2flac)
