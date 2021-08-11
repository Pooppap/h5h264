cmake_minimum_required(VERSION 2.8...3.21)

if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW) # CMake 3.0
endif()

IF(CMAKE_COMPILER_IS_GNUCC)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99")
ENDIF(CMAKE_COMPILER_IS_GNUCC)

project (h5h264)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

SET(CMAKE_INSTALL_PREFIX "/usr/local/hdf5/lib")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message("Building library wtih debug flags")
  add_definitions(-DH5PY_H264_DEBUG=1)
endif()

find_package(FFMPEG)
include_directories(${FFMPEG_INCLUDE_DIR})

find_package(HDF5)
include_directories(${HDF5_INCLUDE_DIR})

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(tests)

# uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()

message("Installing HDF5 filter to ${CMAKE_INSTALL_PREFIX}/plugin")
