set(HG_SQUIRREL_BINDING_DIR ${CMAKE_CURRENT_BINARY_DIR}/binding)

add_custom_command(
	OUTPUT
		${HG_SQUIRREL_BINDING_DIR}/bind_Squirrel.cpp
		${HG_SQUIRREL_BINDING_DIR}/bind_Squirrel.h
		${HG_SQUIRREL_BINDING_DIR}/fabgen.h
	COMMAND
		${Python3_EXECUTABLE} bind.py ${CMAKE_CURRENT_SOURCE_DIR}/../../binding/bind_harfang.py --squirrel --out ${HG_SQUIRREL_BINDING_DIR} ${HG_BINDING_DEFINES}
	MAIN_DEPENDENCY
		${CMAKE_CURRENT_SOURCE_DIR}/../../binding/bind_harfang.py
	DEPENDS
		${HG_FABGEN_PATH}/bind.py
		${HG_FABGEN_PATH}/lang/squirrel.py
		${HG_FABGEN_PATH}/lib/squirrel/std.py
		${HG_FABGEN_PATH}/lib/squirrel/stl.py
	WORKING_DIRECTORY
		${HG_FABGEN_PATH}
	COMMENT
		"Generating Harfang binding for Squirrel extension")

add_library(hg_squirrel SHARED
	${HG_SQUIRREL_BINDING_DIR}/bind_Squirrel.cpp
	${HG_SQUIRREL_BINDING_DIR}/bind_Squirrel.h
	${HG_SQUIRREL_BINDING_DIR}/fabgen.h)
target_include_directories(hg_squirrel PUBLIC ${HG_SQUIRREL_PATH}/include)
target_link_libraries(hg_squirrel engine foundation platform squirrel)
set_target_properties(hg_squirrel PROPERTIES OUTPUT_NAME "harfang" PREFIX "" DEBUG_POSTFIX "" FOLDER "harfang/languages")
if(APPLE)
	set_target_properties(hg_squirrel PROPERTIES SUFFIX ".so")
endif()

add_executable(hg_squirrel_launcher launcher.cpp)
target_include_directories(hg_squirrel_launcher PUBLIC ${HG_SQUIRREL_PATH}/include)
target_link_libraries(hg_squirrel_launcher squirrel sqstdlib ${CMAKE_DL_LIBS})
set_target_properties(hg_squirrel_launcher PROPERTIES OUTPUT_NAME "hg_squirrel" DEBUG_POSTFIX "" FOLDER "harfang/languages")
add_dependencies(hg_squirrel_launcher hg_squirrel)

set(_hg_squirrel_app_launcher_includes
	${CMAKE_CURRENT_SOURCE_DIR}/../../harfang
	${CMAKE_CURRENT_SOURCE_DIR}/../../extern
	${HG_SQUIRREL_PATH}/include)
set(_hg_squirrel_app_launcher_sources
	launcher_app.cpp
	../launcher_app_common.cpp)
set(_hg_squirrel_app_launcher_libs engine foundation platform jsonhpp squirrel sqstdlib ${CMAKE_DL_LIBS})

add_executable(launcher_squirrel ${_hg_squirrel_app_launcher_sources})
target_include_directories(launcher_squirrel PUBLIC ${_hg_squirrel_app_launcher_includes})
target_link_libraries(launcher_squirrel ${_hg_squirrel_app_launcher_libs})
set_target_properties(launcher_squirrel PROPERTIES OUTPUT_NAME "launcher_squirrel" DEBUG_POSTFIX "" FOLDER "harfang/languages")
add_dependencies(launcher_squirrel hg_squirrel)

if(WIN32)
	add_executable(launcher_squirrel_noconsole WIN32 ${_hg_squirrel_app_launcher_sources})
	target_include_directories(launcher_squirrel_noconsole PUBLIC ${_hg_squirrel_app_launcher_includes})
	target_link_libraries(launcher_squirrel_noconsole ${_hg_squirrel_app_launcher_libs})
	target_compile_definitions(launcher_squirrel_noconsole PRIVATE HG_SQUIRREL_LAUNCHER_NO_CONSOLE)
	set_target_properties(launcher_squirrel_noconsole PROPERTIES OUTPUT_NAME "launcher_squirrel_noconsole" DEBUG_POSTFIX "" FOLDER "harfang/languages")
	add_dependencies(launcher_squirrel_noconsole hg_squirrel)
endif()

if(WIN32)
	set_target_properties(hg_squirrel PROPERTIES COMPILE_FLAGS /bigobj)
endif()

install(TARGETS hg_squirrel hg_squirrel_launcher launcher_squirrel squirrel sqstdlib
	RUNTIME DESTINATION hg_squirrel COMPONENT squirrel
	LIBRARY DESTINATION hg_squirrel COMPONENT squirrel
	ARCHIVE DESTINATION hg_squirrel COMPONENT squirrel)
if(WIN32)
	install(TARGETS launcher_squirrel_noconsole RUNTIME DESTINATION hg_squirrel COMPONENT squirrel)
endif()
install_cppsdk_dependencies(hg_squirrel squirrel)
install(FILES bin.nut DESTINATION hg_squirrel/harfang COMPONENT squirrel)
