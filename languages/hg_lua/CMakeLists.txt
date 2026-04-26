add_custom_command(
	OUTPUT
		${CMAKE_CURRENT_BINARY_DIR}/binding/bind_Lua.cpp
		${CMAKE_CURRENT_BINARY_DIR}/binding/bind_Lua.h
		${CMAKE_CURRENT_BINARY_DIR}/binding/fabgen.h
	COMMAND
		${Python3_EXECUTABLE} bind.py ${CMAKE_CURRENT_SOURCE_DIR}/../../binding/bind_harfang.py --lua --out ${CMAKE_CURRENT_BINARY_DIR}/binding ${HG_BINDING_DEFINES}
	MAIN_DEPENDENCY
		${CMAKE_CURRENT_SOURCE_DIR}/../../binding/bind_harfang.py
	WORKING_DIRECTORY
		${HG_FABGEN_PATH}
	COMMENT
		"Generating Harfang binding for Lua extension")

add_library(hg_lua SHARED
	assets_bridge.cpp
	assets_bridge.h
	${CMAKE_CURRENT_BINARY_DIR}/binding/bind_Lua.cpp
	${CMAKE_CURRENT_BINARY_DIR}/binding/bind_Lua.h
	${CMAKE_CURRENT_BINARY_DIR}/binding/fabgen.h)
target_compile_definitions(hg_lua PRIVATE HG_LUA_BUILD_DLL)
target_include_directories(hg_lua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../extern/lua/src)
target_link_libraries(hg_lua engine foundation platform)
set_target_properties(hg_lua PROPERTIES OUTPUT_NAME "harfang" PREFIX "" DEBUG_POSTFIX "")
if(APPLE)
	set_target_properties(hg_lua PROPERTIES SUFFIX ".so")
endif()
set_target_properties(hg_lua PROPERTIES FOLDER "harfang/languages")

add_dependencies(hg_lua lua)

set(_hg_lua_launcher_includes
	${CMAKE_CURRENT_SOURCE_DIR}/../../harfang
	${CMAKE_CURRENT_SOURCE_DIR}/../../extern
	${CMAKE_CURRENT_SOURCE_DIR}/../../extern/lua/src)
set(_hg_lua_launcher_libs hg_lua engine foundation platform jsonhpp libluadll)

add_executable(launcher launcher.cpp)
target_include_directories(launcher
	PUBLIC
		${_hg_lua_launcher_includes})
target_link_libraries(launcher ${_hg_lua_launcher_libs})
set_target_properties(launcher PROPERTIES OUTPUT_NAME "launcher" DEBUG_POSTFIX "" FOLDER "harfang/languages")
add_dependencies(launcher hg_lua)

if(WIN32)
	add_executable(launcher_noconsole WIN32 launcher.cpp)
	target_include_directories(launcher_noconsole
		PUBLIC
			${_hg_lua_launcher_includes})
	target_link_libraries(launcher_noconsole ${_hg_lua_launcher_libs})
	target_compile_definitions(launcher_noconsole PRIVATE HG_LUA_LAUNCHER_NO_CONSOLE)
	set_target_properties(launcher_noconsole PROPERTIES OUTPUT_NAME "launcher_noconsole" DEBUG_POSTFIX "" FOLDER "harfang/languages")
	add_dependencies(launcher_noconsole hg_lua)
endif()

if(WIN32)
	set_target_properties(hg_lua PROPERTIES COMPILE_FLAGS /bigobj)
elseif(UNIX)
	target_compile_definitions(hg_lua PUBLIC LUA_USE_LINUX)
endif()

install(TARGETS hg_lua lua launcher RUNTIME DESTINATION hg_lua LIBRARY DESTINATION hg_lua COMPONENT lua)
if(WIN32)
	install(TARGETS launcher_noconsole RUNTIME DESTINATION hg_lua COMPONENT lua)
endif()

install_cppsdk_dependencies(hg_lua lua)

foreach(_target assetc fbx_converter assimp_converter gltf_importer gltf_exporter)
	if(TARGET ${_target})
		install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/hg_lua/harfang -DBUILD_TYPE=$<CONFIG> -P $<TARGET_PROPERTY:${_target},BINARY_DIR>/cmake_install.cmake)"
			COMPONENT lua)
	endif()
endforeach()

install(FILES bin.lua DESTINATION hg_lua/harfang COMPONENT lua)
