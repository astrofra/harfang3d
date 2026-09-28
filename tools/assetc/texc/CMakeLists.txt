cmake_minimum_required(VERSION 3.16.2)

project(texc LANGUAGES CXX)

find_package(Threads REQUIRED)

add_library(bc7enc STATIC
	bc7enc/lodepng.cpp
	bc7enc/lodepng.h
	bc7enc/bc7decomp_ref.cpp
	bc7enc/bc7decomp.cpp
	bc7enc/bc7decomp.h
	bc7enc/bc7enc.cpp
	bc7enc/bc7enc.h
	bc7enc/ert.cpp
	bc7enc/ert.h
	bc7enc/rgbcx.cpp
	bc7enc/rgbcx.h
	bc7enc/utils.cpp
	bc7enc/utils.h
	bc7enc/rdo_bc_encoder.cpp
	bc7enc/rdo_bc_encoder.h
)

add_executable(texc main.cpp cmd_line.cpp cmd_line.h)
target_compile_definitions(texc PRIVATE FMT_HEADER_ONLY)
target_compile_features(texc PRIVATE cxx_std_14)
target_compile_features(bc7enc PRIVATE cxx_std_14)
target_link_libraries(texc PRIVATE bc7enc Threads::Threads)
set_target_properties(texc bc7enc PROPERTIES FOLDER "harfang/tools/texc")

# Harfang disables optimization globally in RelWithDebInfo (the default on Unix).
# Keep the offline texture compiler fast without changing engine/debug builds.
foreach(target texc bc7enc)
	if(MSVC)
		target_compile_options(${target} PRIVATE $<$<CONFIG:RelWithDebInfo>:/O2> $<$<CONFIG:RelWithDebInfo>:/Ob2>)
	else()
		target_compile_options(${target} PRIVATE $<$<CONFIG:RelWithDebInfo>:-O2>)
	endif()
endforeach()
