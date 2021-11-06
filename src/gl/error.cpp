/****************************************************************
**error.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-26.
*
* Description: Error checking/handling for OpenGL.
*
*****************************************************************/
#include "error.hpp"

// Glad
#include "glad/glad.h"

// C++ standard library
#include <iostream>

using namespace std;

namespace gl {

bool print_errors() {
  GLenum err_code;
  bool   error_found = false;
  while( true ) {
    err_code = glGetError();
    if( err_code == GL_NO_ERROR ) break;
    cerr << "OpenGL error: " << err_code << "\n";
    error_found = true;
  }
  return error_found;
}

void check_errors() {
  if( !print_errors() ) return;
  FATAL( "Aborting after one or more OpenGL errors occurred." );
}

} // namespace gl
