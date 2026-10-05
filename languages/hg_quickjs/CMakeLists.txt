file(GLOB_RECURSE fabgen_sources CONFIGURE_DEPENDS "${HG_FABGEN_PATH}/*.py" "${HG_FABGEN_PATH}/*.h")
set(out "${CMAKE_CURRENT_BINARY_DIR}/binding")
add_custom_command(OUTPUT "${out}/bind_QuickJS.cpp" "${out}/bind_QuickJS.h" "${out}/fabgen.h" "${out}/fabgen_quickjs.h"
    COMMAND "${Python3_EXECUTABLE}" bind.py "${PROJECT_SOURCE_DIR}/binding/bind_harfang.py" --quickjs --embedded --prefix hg_quickjs --out "${out}" ${HG_BINDING_DEFINES}
    DEPENDS "${PROJECT_SOURCE_DIR}/binding/bind_harfang.py" "${PROJECT_SOURCE_DIR}/binding/quickjs_lua.py" ${fabgen_sources}
    WORKING_DIRECTORY "${HG_FABGEN_PATH}" VERBATIM)

# An external language binding. The engine and its embedded Lua VM are unchanged.
add_library(hg_quickjs STATIC "${out}/bind_QuickJS.cpp" quickjs_vm.cpp quickjs_vm.h)
target_link_libraries(hg_quickjs PUBLIC hg_quickjs_runtime engine)
target_include_directories(hg_quickjs PUBLIC "${out}" "${CMAKE_CURRENT_SOURCE_DIR}")
if(MSVC)
    target_compile_options(hg_quickjs PRIVATE /bigobj)
endif()
set_target_properties(hg_quickjs PROPERTIES FOLDER "harfang/languages")

add_executable(HarfangJs launcher.cpp)
target_link_libraries(HarfangJs PRIVATE hg_quickjs)
set_target_properties(HarfangJs PROPERTIES FOLDER "harfang/languages" OUTPUT_NAME "hgjs" DEBUG_POSTFIX "")
# Preserve the initial build helper's target spelling.
add_custom_target(hgjs DEPENDS HarfangJs)
if(WIN32 AND NOT HG_BUILD_HARFANG_STATIC)
    foreach(dependency glfw libluadll)
        add_custom_command(TARGET HarfangJs POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${dependency}>" "$<TARGET_FILE_DIR:HarfangJs>" VERBATIM)
    endforeach()
endif()
if(HG_BUILD_TESTS)
    add_executable(hg_quickjs_tests tests.cpp quickjs_vm.cpp)
    target_link_libraries(hg_quickjs_tests PRIVATE hg_quickjs_runtime)
    target_include_directories(hg_quickjs_tests PRIVATE "${PROJECT_SOURCE_DIR}/extern/acutest")
    set_target_properties(hg_quickjs_tests PROPERTIES FOLDER "harfang/languages")
endif()

install(TARGETS HarfangJs RUNTIME DESTINATION hgjs COMPONENT quickjs)
if(TARGET audio_xmp)
    add_dependencies(HarfangJs audio_xmp)
    install(TARGETS audio_xmp
        RUNTIME DESTINATION hgjs COMPONENT quickjs
        LIBRARY DESTINATION hgjs COMPONENT quickjs)
endif()
install_cppsdk_dependencies(hgjs quickjs)
install(FILES README.md "${PROJECT_SOURCE_DIR}/LICENSE" "${PROJECT_SOURCE_DIR}/LICENSE.GPL3" "${PROJECT_SOURCE_DIR}/LICENSE-3RD-PARTIES" DESTINATION hgjs COMPONENT quickjs)
