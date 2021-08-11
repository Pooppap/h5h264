add_library(h264_filter SHARED h264_filter.c)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(h264_filter ${FFMPEG_LIBRARIES})
target_link_libraries(h264_filter ${HDF5_LIBRARIES})

install(TARGETS h264_filter DESTINATION plugin)