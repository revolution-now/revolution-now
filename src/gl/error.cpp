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

// gl
#include "iface.hpp"

// base
#include "base/fmt.hpp"

// C++ standard library
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace gl {

vector<string> has_errors() {
  vector<string> errors;
  GLenum         err_code;
  while( true ) {
    err_code = CALL_GL( gl_GetError );
    if( err_code == GL_NO_ERROR ) break;
    errors.push_back(
        fmt::format( "OpenGL error: {}", err_code ) );
  }
  return errors;
}

bool print_errors() {
  vector<string> errors = has_errors();
  for( string const& error : errors ) cerr << error;
  return !errors.empty();
}

void check_errors() {
  if( !print_errors() ) return;
  FATAL( "Aborting after one or more OpenGL errors occurred." );
}

} // namespace gl
