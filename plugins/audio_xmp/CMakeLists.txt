add_library(audio_xmp SHARED audio_xmp.cpp)

target_include_directories(audio_xmp PRIVATE ${PROJECT_SOURCE_DIR}/harfang)
target_link_libraries(audio_xmp PRIVATE xmp_lite_static)

set_target_properties(audio_xmp PROPERTIES PREFIX "" DEBUG_POSTFIX "" FOLDER "harfang/plugins")

install_cppsdk_target(audio_xmp)

if(HG_BUILD_HG_LUA)
	install(TARGETS audio_xmp RUNTIME DESTINATION hg_lua LIBRARY DESTINATION hg_lua COMPONENT lua)
endif()
