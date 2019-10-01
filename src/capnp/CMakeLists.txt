set( CapnProto_ROOT "/home/dsicilia/dev/tools/capnproto-current" )
find_package( CapnProto REQUIRED )

file( GLOB capnps "[a-zA-Z]*.capnp" )

foreach( cp ${capnps} )
  capnp_generate_cpp( CAPNP_SRCS CAPNP_HDRS ${cp} )
endforeach()

add_library( rn-capnp STATIC ${CAPNP_SRCS} )

target_link_libraries(
  rn-capnp
  PUBLIC
  CapnProto::capnp
)

target_include_directories(
  rn-capnp
  PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR} # for dependent targets.
)

set_target_properties( rn-capnp PROPERTIES CXX_EXTENSIONS OFF )
target_compile_features( rn-capnp PUBLIC cxx_std_20 )
set_warning_options( rn-capnp )
