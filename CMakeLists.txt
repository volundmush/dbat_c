cmake_minimum_required(VERSION 3.28)

project(circle)

file(GLOB CIRCLE_INCLUDE src/*.h)
file(GLOB CIRCLE_SRC src/*.c)

set_property(GLOBAL PROPERTY C_STANDARD 23)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

find_library(LIBBSD NAMES bsd libbsd)
include_directories(include)
if(${LIBBSD})
    link_libraries(${LIBBSD})
    add_compile_definitions(HAVE_LIBBSD)
endif()

find_library(ZLIB NAMES z libz REQUIRED)
link_libraries(${ZLIB})

# this is the core library we're making.
add_library(circlemud ${CIRCLE_INCLUDE} ${CIRCLE_SRC})
# everything we make will link to it.
link_libraries(circlemud)

# the various binaries
add_executable(circle apps/circle.c)

target_compile_definitions(circle PUBLIC USING_CMAKE=1 CIRCLE_UNIX=1 POSIX=1)

SET(circle_link ${CMAKE_INSTALL_PREFIX}/bin/)