add_executable(legacy_archive legacy_archive.c)

set_target_properties(legacy_archive PROPERTIES FOLDER "harfang/tools" C_STANDARD 99)
target_link_libraries(legacy_archive miniz)
