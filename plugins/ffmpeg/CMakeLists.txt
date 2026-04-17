list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(MSVC AND NOT FFMPEG_ROOT)
	message(FATAL_ERROR "FFMPEG_ROOT is required to build hg_ffmpeg with MSVC")
endif()

find_package(Threads REQUIRED)
find_package(FFMPEG REQUIRED COMPONENTS avutil avcodec avformat avdevice swscale)

set(HG_FFMPEG_SRCS
	ffmpeg_video_stream.cpp
)

set(HG_FFMPEG_HDRS
	${PROJECT_SOURCE_DIR}/harfang/engine/video_stream_interface.h
)

add_library(hg_ffmpeg SHARED ${HG_FFMPEG_SRCS} ${HG_FFMPEG_HDRS})
target_include_directories(hg_ffmpeg
	PRIVATE
		${PROJECT_SOURCE_DIR}/harfang/engine
)
target_link_libraries(hg_ffmpeg
	PRIVATE
		FFMPEG::avutil
		FFMPEG::avcodec
		FFMPEG::avformat
		FFMPEG::avdevice
		FFMPEG::swscale
		Threads::Threads
)

set_target_properties(hg_ffmpeg PROPERTIES
	PREFIX ""
	FOLDER "plugins"
)

install(TARGETS hg_ffmpeg
	RUNTIME DESTINATION ${CppSdkRuntimeDestination}
	LIBRARY DESTINATION ${CppSdkRuntimeDestination}
	COMPONENT cppsdk
)
