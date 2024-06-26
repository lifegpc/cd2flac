cmake_minimum_required(VERSION 3.17)

project(cd2flac)

if (MSVC)
    add_compile_options(/utf-8)
endif()

option(ENABLE_INIH "Enable ini support with inih." ON)

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
find_package(OpenSSL 3.0.0 REQUIRED)
find_package(RapidJSON)

if (ENABLE_INIH)
    find_package(INIH REQUIRED)
    set(HAVE_INIH 1)
endif()

if (WIN32)
    check_symbol_exists(printf_s "stdio.h" HAVE_PRINTF_S)
    check_symbol_exists(sscanf_s "stdio.h" HAVE_SSCANF_S)
    check_symbol_exists(scanf_s "stdio.h" HAVE_SCANF_S)
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h")

CHECK_INCLUDE_FILES(getopt.h HAVE_GETOPT_H)
if (NOT HAVE_GETOPT_H)
    add_subdirectory(getopt)
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/getopt")
endif()

set(ENABLE_ICONV OFF CACHE BOOL "Libiconv is not needed.")
set(ENABLE_SSL ON)
set(ENABLE_ZLIB ON CACHE BOOL "Enable zlib by default.")
add_subdirectory(utils)
include_directories(utils)

include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/utils")

add_custom_target(cd2flac_version
    ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_SOURCE_DIR}/src/VERSION.cmake"
)

if (WIN32)
    set(CD2FLAC_RC "${CMAKE_CURRENT_BINARY_DIR}/cd2flac.rc")
    set_source_files_properties(${CD2FLAC_RC} PROPERTIES GENERATED TRUE)
endif()

if (HAVE_INIH)
    list(APPEND OFILES src/INIReader.cpp src/INIReader.h)
endif()

add_executable(cd2flac
src/main.cpp src/info.h src/info.c src/core.h src/core.cpp src/open.h src/open.c
src/cue.h src/cue.cpp src/cue_track_list.h src/cue_track_list.cpp src/163.h
src/163.cpp src/music_brain_z.h src/music_brain_z.cpp "${OFILES}" ${CD2FLAC_RC})
add_dependencies(cd2flac cd2flac_version)
target_compile_definitions(cd2flac PRIVATE -DHAVE_CONFIG_H)
if (TARGET getopt)
    target_link_libraries(cd2flac getopt)
endif()
target_link_libraries(cd2flac AVUTIL::AVUTIL AVFORMAT::AVFORMAT AVCODEC::AVCODEC AVDEVICE::AVDEVICE SWRESAMPLE::SWRESAMPLE CDDB::CDDB OpenSSL::Crypto RapidJSON)
if (HAVE_INIH)
    target_link_libraries(cd2flac INIH::INIH)
endif()
target_link_libraries(cd2flac utils)

install(TARGETS cd2flac)
