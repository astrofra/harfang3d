add_executable(gamestart_archive gamestart_archive.c)

set_target_properties(gamestart_archive PROPERTIES FOLDER "harfang/tools" C_STANDARD 99)
target_link_libraries(gamestart_archive miniz)
