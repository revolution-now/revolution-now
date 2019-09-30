set( Protobuf_USE_STATIC_LIBS ON )

find_package( Protobuf REQUIRED )

file( GLOB protos "[a-zA-Z]*.proto" )

foreach( pb ${protos} )
  protobuf_generate_cpp( PROTO_SRCS PROTO_HDRS ${pb} )
endforeach()

add_library( rn-proto STATIC ${PROTO_SRCS} )

target_link_libraries(
  rn-proto
  PUBLIC
  ${Protobuf_LIBRARIES}
)

target_include_directories(
  rn-proto
  PUBLIC
  ${Protobuf_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR} # for dependent targets.
)

set_target_properties( rn-proto PROPERTIES CXX_EXTENSIONS OFF )
target_compile_features( rn-proto PUBLIC cxx_std_20 )
set_warning_options( rn-proto )
