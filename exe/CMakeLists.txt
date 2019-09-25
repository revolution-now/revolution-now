add_executable( exe main.cpp )
set_warning_options( exe )

target_link_libraries(
    exe
    PUBLIC
    rn
)

# The `midi-convert` target will convert (incrementally) any .rg
# (Rosegarden) files to MIDI.  The `ogg-convert` will render the
# midi to OGG.
if( NOT ENABLE_ADDRESS_SANITIZER )
  # FIXME: we can only enable these if address sanitizer is off,
  #        because the rg2midi program does not pass ASan.
  add_dependencies( exe midi-convert )
  add_dependencies( exe ogg-convert )
endif()

clang_tidy( exe )
