# This was downloaded as a C file then renamd to CPP.
set( sources src/glad.cpp )

add_library( glad STATIC ${sources} )
add_library( glad::glad ALIAS glad )

target_compile_features( glad PUBLIC cxx_std_20 )
set_target_properties( glad PROPERTIES CXX_EXTENSIONS OFF )
set_warning_options( glad )

# This might technically be needed, but I think we're linking it
# in somewhere else.
#target_link_libraries( glad PUBLIC dl )

target_include_directories(
  glad
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
