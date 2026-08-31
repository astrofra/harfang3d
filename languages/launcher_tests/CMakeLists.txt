set(_launcher_regression_targets)
set(_launcher_regression_tests)

function(add_launcher_regression_test test_name language mode expected_source launcher_target launcher_install_dir launcher_install_name)
	set(_test_command
		${Python3_EXECUTABLE}
		${CMAKE_CURRENT_SOURCE_DIR}/run_launcher_regression.py
		--language ${language}
		--mode ${mode}
		--expected-source ${expected_source}
		--launcher ${CMAKE_INSTALL_PREFIX}/${launcher_install_dir}/${launcher_install_name}${CMAKE_EXECUTABLE_SUFFIX}
		--fixture-root ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/${language}
		--work-dir ${CMAKE_CURRENT_BINARY_DIR}/work/${test_name})

	if(mode STREQUAL "legacy")
		list(APPEND _test_command --packer $<TARGET_FILE:legacy_archive>)
		list(APPEND _launcher_regression_targets legacy_archive)
	endif()

	add_test(NAME ${test_name} COMMAND ${_test_command})

	set_tests_properties(${test_name} PROPERTIES LABELS "launcher_regression;launcher_regression_${language};launcher_regression_${mode}")
	list(APPEND _launcher_regression_tests ${test_name})
	set(_launcher_regression_tests "${_launcher_regression_tests}" PARENT_SCOPE)

	list(APPEND _launcher_regression_targets ${launcher_target})
	set(_launcher_regression_targets "${_launcher_regression_targets}" PARENT_SCOPE)
endfunction()

if(BUILD_TESTING)
	if(TARGET launcher)
		add_launcher_regression_test(launcher_lua_folder lua folder folder launcher hg_lua launcher)
	endif()

	if(TARGET launcher AND TARGET legacy_archive)
		add_launcher_regression_test(launcher_lua_legacy lua legacy legacy launcher hg_lua launcher)
	endif()

	if(TARGET launcher_squirrel)
		add_launcher_regression_test(launcher_squirrel_folder squirrel folder folder launcher_squirrel hg_squirrel launcher_squirrel)
	endif()

	if(TARGET launcher_squirrel AND TARGET legacy_archive)
		add_launcher_regression_test(launcher_squirrel_legacy squirrel legacy legacy launcher_squirrel hg_squirrel launcher_squirrel)
	endif()
endif()

if(_launcher_regression_tests)
	list(REMOVE_DUPLICATES _launcher_regression_targets)
	add_custom_target(launcher_regression
		COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config $<CONFIG>
		COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L launcher_regression -C $<CONFIG>
		DEPENDS ${_launcher_regression_targets}
		USES_TERMINAL
		COMMENT "Running launcher regression suite")
endif()
