file( GLOB fbs "[a-z][a-z0-9-]*.fbs" )

build_flatbuffers(
  "${fbs}"                    # flatbuffers_schemas
  ${CMAKE_CURRENT_SOURCE_DIR} # schema_include_dirs
  flatbuf                     # custom_target_name
  ""                          # additional_dependencies
  ${CMAKE_CURRENT_BINARY_DIR} # generated_includes_dir
  ""                          # binary_schemas_dir
  ""                          # copy_text_schemas_dir
)
