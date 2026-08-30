if(HG_BUILD_HG_LUA)
	add_subdirectory(hg_lua)
endif()

if(HG_BUILD_HG_SQUIRREL)
	add_subdirectory(hg_squirrel)
endif()

if(HG_BUILD_HG_PYTHON)
	add_subdirectory(hg_python)
endif()

if(HG_BUILD_HG_GO)
	add_subdirectory(hg_go)
endif()
