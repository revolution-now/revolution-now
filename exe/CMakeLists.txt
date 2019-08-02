add_executable( exe main.cpp )
set_warning_options( exe )

target_link_libraries(
    exe
    PUBLIC
    rn
)

target_include_directories(
    exe
    PUBLIC
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src
)

# The `midi-convert` target will convert (incrementally) any .rg
# (Rosegarden) files to MIDI.  The `ogg-convert` will render the
# midi to OGG.
add_dependencies( exe midi-convert )
add_dependencies( exe ogg-convert )

clang_tidy( exe )
