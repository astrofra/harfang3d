add_library(lzma STATIC Alloc.c CpuArch.c LzFind.c LzmaDec.c LzmaEnc.c)
set_target_properties(lzma PROPERTIES C_STANDARD 99 POSITION_INDEPENDENT_CODE TRUE FOLDER "harfang/3rdparty")
target_compile_definitions(lzma PRIVATE Z7_ST)
target_include_directories(lzma PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
