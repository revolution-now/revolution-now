/****************************************************************
**iface.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-10.
*
* Description: Virtual interface for OpenGL methods.
*
*****************************************************************/
#include "iface.hpp"

namespace gl {

namespace {

IOpenGL* g_impl = nullptr;

} // namespace

/****************************************************************
** Public API
*****************************************************************/
IOpenGL* global_gl_implementation() { return g_impl; }

void set_global_gl_implementation( IOpenGL* iopengl ) {
  g_impl = iopengl;
}

} // namespace gl
