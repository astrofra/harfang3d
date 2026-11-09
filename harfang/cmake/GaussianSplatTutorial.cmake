# Native, single-backend distribution of the unchanged Gaussian Lua tutorial.
if(CMAKE_CROSSCOMPILING OR NOT (APPLE OR WIN32))
	return()
endif()
if(APPLE AND HG_GRAPHIC_API STREQUAL "MTL")
	set(_gsplat_platform osx)
elseif(WIN32 AND HG_GRAPHIC_API STREQUAL "DX11")
	set(_gsplat_platform windows)
else()
	return() # The reference tutorial explicitly selects Metal / D3D11.
endif()

set(_gsplat_root "${CMAKE_BINARY_DIR}/gaussian_splatting/$<CONFIG>")
install(TARGETS lua hg_lua
	RUNTIME DESTINATION . COMPONENT gsplat_tutorial_runtime
	LIBRARY DESTINATION . COMPONENT gsplat_tutorial_runtime)
install_cppsdk_dependencies(. gsplat_tutorial_runtime)
set(_gsplat_asset_target "${HG_TARGET_PREFIX}")
if(ASSETC_TARGET_PREFIX)
	set(_gsplat_asset_target "${ASSETC_TARGET_PREFIX}")
endif()

set(_gsplat_tools assetc)
foreach(_tool recastc bulletc legacy_archive)
	if(TARGET ${_tool})
		list(APPEND _gsplat_tools ${_tool})
	endif()
endforeach()
add_custom_target(gsplat_tutorial_package
	COMMAND "${CMAKE_COMMAND}"
		"-DREPO=${CMAKE_SOURCE_DIR}" "-DBUILD=${CMAKE_BINARY_DIR}"
		"-DCONFIG=$<CONFIG>" "-DROOT=${_gsplat_root}"
		"-DAPI=${HG_GRAPHIC_API}" "-DPLATFORM=${_gsplat_platform}"
		"-DARCH=${CMAKE_SYSTEM_PROCESSOR}"
		"-DTOOLCHAIN=host-${HG_TARGET_PREFIX}-target-${_gsplat_asset_target}"
		-P "${CMAKE_SOURCE_DIR}/tools/package_gaussian_splatting.cmake"
	DEPENDS hg_lua lua ${_gsplat_tools}
	USES_TERMINAL VERBATIM)
add_custom_target(gsplat_tutorial_run
	COMMAND "${CMAKE_COMMAND}" -E chdir "${_gsplat_root}/app"
		"${CMAKE_COMMAND}" -E env "LUA_CPATH=./?.so;./?.dll;;"
		"${_gsplat_root}/app/$<TARGET_FILE_NAME:lua>" gaussian_splatting.lua
	DEPENDS gsplat_tutorial_package USES_TERMINAL VERBATIM)
add_custom_target(gsplat_tutorial_smoke
	COMMAND "${CMAKE_COMMAND}" -E chdir "${_gsplat_root}/app"
		"${_gsplat_root}/app/$<TARGET_FILE_NAME:lua>"
		"${CMAKE_SOURCE_DIR}/harfang/tests/gaussian_splatting_runner.lua"
		. gaussian_splatting.lua 60 "${_gsplat_root}/tutorial"
	COMMAND "${CMAKE_COMMAND}" -E chdir "${_gsplat_root}/app"
		"${_gsplat_root}/app/$<TARGET_FILE_NAME:lua>"
		"${CMAKE_SOURCE_DIR}/harfang/tests/gaussian_splatting_runner.lua"
		. gaussian_splatting.lua 60 "${_gsplat_root}/tutorial-space" space
	DEPENDS gsplat_tutorial_package USES_TERMINAL VERBATIM)
