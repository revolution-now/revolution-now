/****************************************************************
**vertex-array.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-27.
*
* Description: Class to encapsulate OpenGL Vertex Arrays.
*
*****************************************************************/
#include "vertex-array.hpp"

// gl
#include "error.hpp"

// base
#include "base/error.hpp"
#include "base/fmt.hpp"

// Glad
#include "glad/glad.h"

using namespace std;

namespace gl {

/****************************************************************
** VertexArrayNonTyped
*****************************************************************/
VertexArrayNonTyped::VertexArrayNonTyped() {
  ObjId vao_id = 0;
  GL_CHECK( glGenVertexArrays( 1, &vao_id ) );
  *this = VertexArrayNonTyped( vao_id );
}

VertexArrayNonTyped::VertexArrayNonTyped( ObjId vao_id )
  : base::zero<VertexArrayNonTyped, ObjId>( vao_id ),
    bindable<VertexArrayNonTyped>() {}

void VertexArrayNonTyped::free_resource() {
  ObjId vao_id = resource();
  DCHECK( vao_id != 0 );
  GL_CHECK( glDeleteVertexArrays( 1, &vao_id ) );
}

ObjId VertexArrayNonTyped::current_bound() {
  GLint id;
  GL_CHECK( glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &id ) );
  return (ObjId)id;
}

void VertexArrayNonTyped::bind_obj_id( ObjId new_id ) {
  GL_CHECK( glBindVertexArray( new_id ) );
}

void VertexArrayNonTyped::register_attrib(
    int idx, size_t attrib_field_count,
    e_vertex_attrib_type type, bool normalized, size_t stride,
    size_t offset ) const {
  GLboolean gl_normalized = normalized ? GL_TRUE : GL_FALSE;
  GLenum    gl_type       = 0;
  switch( type ) {
    case e_vertex_attrib_type::float_: gl_type = GL_FLOAT; break;
  }
  GL_CHECK( glVertexAttribPointer( idx, attrib_field_count,
                                   gl_type, gl_normalized,
                                   stride, (void*)offset ) );
  GL_CHECK( glEnableVertexAttribArray( idx ) );
}

} // namespace gl
