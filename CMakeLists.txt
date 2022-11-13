cmake_minimum_required(VERSION 2.8)

project(circle)

file(GLOB CIRCLE_INCLUDE src/*.h)
file(GLOB CIRCLE_SRC src/*.c)

set_property(GLOBAL PROPERTY C_STANDARD 11)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

find_library(LIBBSD NAMES bsd libbsd)
include_directories(include)
if(${LIBBSD})
    link_libraries(${LIBBSD})
    add_compile_definitions(HAVE_LIBBSD)
endif()

# this is the core library we're making.
add_library(circlemud ${CIRCLE_INCLUDE} ${CIRCLE_SRC})
# everything we make will link to it.
link_libraries(circlemud)

# the various binaries
add_executable(circle apps/circle.c amalgam.h include/dg_mobcmd.h)
#add_executable(asciipasswd apps/circle.c)
#add_executable(autowiz apps/autowiz.c)
#add_executable(delobjs apps/delobjs.c)
#add_executable(play2to3 apps/play2to3.c)
#add_executable(shopconv apps/shopconv.c)
#add_executable(showplay apps/showplay.c)
#add_executable(sign apps/sign.c)
#add_executable(split apps/split.c)
#add_executable(wld2html apps/wld2html.c)

target_compile_definitions(circle PUBLIC USING_CMAKE=1 CIRCLE_UNIX=1 POSIX=1)


#find_package(SQLite3 REQUIRED)
#if(SQLite3_FOUND)
#    message("Found SQLite3.")
#    target_link_libraries(circlemud SQLite::SQLite3)
#endif()

#find_package(OpenSSL REQUIRED)
#if(OpenSSL_FOUND)
#    message("Found OpenSSL.")
#    target_link_libraries(circlemud OpenSSL::Crypto)
#endif()

# Support for FMT
#target_include_directories(circlemud PUBLIC libs/fmt/include)
#target_compile_definitions(circlemud PUBLIC FMT_HEADER_ONLY=1)

# Support for SQLITEPP
#target_include_directories(circlemud PUBLIC libs/sqlitepp/include)

# Support for ASIO
#target_include_directories(circlemud PUBLIC libs/asio/asio/include)

SET(circle_link ${CMAKE_INSTALL_PREFIX}/bin/)