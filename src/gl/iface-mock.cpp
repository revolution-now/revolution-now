/****************************************************************
**iface-mock.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-14.
*
* Description: Mock implementation of IOpenGL.
*
*****************************************************************/
#include "iface-mock.hpp"

namespace gl {

MockOpenGL::MockOpenGL() {
  prev_ = global_gl_implementation();
  set_global_gl_implementation( this );
}

MockOpenGL::~MockOpenGL() {
  set_global_gl_implementation( prev_ );
}

} // namespace gl
