cmake_minimum_required(VERSION 2.8)
project(tangram)

# helper functions
include(${PROJECT_SOURCE_DIR}/tangram-es/toolchains/utils.cmake)

# cmake output configuration
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -stdlib=libc++ -std=c++11")
set(CXX_FLAGS_DEBUG "-g -O0")
set(EXECUTABLE_NAME "tangram")

add_definitions(-DPLATFORM_OSX)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

if(NOT GLFW_FOUND)
    message(SEND_ERROR "GLFW not found")
    return()
else()
    include_directories(${GLFW_INCLUDE_DIRS})
    message(STATUS "Found GLFW ${GLFW_PREFIX}")
endif()

# add sources and include headers
set(OSX_EXTENSIONS_FILES *.mm *.cpp)
foreach(_ext ${OSX_EXTENSIONS_FILES})
    find_sources_and_include_directories(
        ${PROJECT_SOURCE_DIR}/src/*.h
        ${PROJECT_SOURCE_DIR}/src/${_ext})
endforeach()

# locate resource files to include
file(GLOB_RECURSE RESOURCES ${PROJECT_SOURCE_DIR}/resources/**)
file(GLOB_RECURSE CORE_RESOURCES ${PROJECT_SOURCE_DIR}/tangram-es/core/resources/**)
list(APPEND RESOURCES ${CORE_RESOURCES})
string(REGEX REPLACE "[.]DS_Store" "" RESOURCES "${RESOURCES}")

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/tangram-es/core)
include_directories(${CORE_INCLUDE_DIRS})
include_directories(${CORE_LIBRARIES_INCLUDE_DIRS})

# add executable
add_executable(${EXECUTABLE_NAME} MACOSX_BUNDLE ${SOURCES} ${RESOURCES})

list(APPEND GLFW_LDFLAGS "-framework OpenGL -framework Cocoa -framework IOKit -framework CoreFoundation -framework CoreVideo")

target_link_libraries(${EXECUTABLE_NAME} core ${GLFW_LDFLAGS})

# add resource files and property list
set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/resources/tangram-Info.plist"
    RESOURCE "${RESOURCES}")

