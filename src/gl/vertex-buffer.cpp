/****************************************************************
**vertex-buffer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-26.
*
* Description: Class to encapsulate OpenGL Vertex Buffers.
*
*****************************************************************/
#include "vertex-buffer.hpp"

// gl
#include "error.hpp"

// base
#include "base/error.hpp"

// Glad
#include "glad/glad.h"

using namespace std;

namespace gl {

auto to_gl_draw_mode( e_draw_mode mode ) {
  switch( mode ) {
    case e_draw_mode::stat1c: return GL_STATIC_DRAW;
    case e_draw_mode::dynamic: return GL_DYNAMIC_DRAW;
  }
}

/****************************************************************
** VertexBufferNonTyped
*****************************************************************/
VertexBufferNonTyped::VertexBufferNonTyped() {
  ObjId vbo_id = 0;
  GL_CHECK( glGenBuffers( 1, &vbo_id ) );
  *this = VertexBufferNonTyped( vbo_id );
}

VertexBufferNonTyped::VertexBufferNonTyped( ObjId vbo_id )
  : base::zero<VertexBufferNonTyped, ObjId>( vbo_id ),
    bindable<VertexBufferNonTyped>() {}

void VertexBufferNonTyped::upload_data_replace_impl(
    void const* data, size_t size, e_draw_mode mode ) const {
  auto binder = bind();
  GL_CHECK( glBufferData( GL_ARRAY_BUFFER, size, data,
                          to_gl_draw_mode( mode ) ) );
}

void VertexBufferNonTyped::upload_data_modify_impl(
    void const* data, size_t size,
    size_t start_offset_bytes ) const {
  auto binder = bind();
  GL_CHECK( glBufferSubData( GL_ARRAY_BUFFER, start_offset_bytes,
                             size, data ) );
}

void VertexBufferNonTyped::free_resource() {
  ObjId vbo_id = resource();
  DCHECK( vbo_id != 0 );
  GL_CHECK( glDeleteBuffers( 1, &vbo_id ) );
}

ObjId VertexBufferNonTyped::current_bound() {
  GLint id;
  GL_CHECK( glGetIntegerv( GL_ARRAY_BUFFER_BINDING, &id ) );
  return (ObjId)id;
}

void VertexBufferNonTyped::bind_obj_id( ObjId new_id ) {
  GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, new_id ) );
}

} // namespace gl
