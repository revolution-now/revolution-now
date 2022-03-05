/****************************************************************
**index-buffer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-05.
*
* Description: RAII type for holding OpenGL index buffers.
*
*****************************************************************/
#include "index-buffer.hpp"

// gl
#include "error.hpp"
#include "iface.hpp"

// C++ standard library
#include <array>

using namespace std;

namespace gl {

/****************************************************************
** IndexBuffer
*****************************************************************/
IndexBuffer::IndexBuffer() {
  ObjId ibo_id = 0;
  GL_CHECK( CALL_GL( gl_GenBuffers, 1, &ibo_id ) );
  *this = IndexBuffer( ibo_id );
}

IndexBuffer::IndexBuffer( ObjId ibo_id )
  : base::zero<IndexBuffer, ObjId>( ibo_id ),
    bindable<IndexBuffer>() {
  using IndexType = unsigned int;
  static array<IndexType, 6> indices{
      0, 1, 3, // first triangle.
      1, 2, 3, // second triangle.
  };
  auto binder = bind();
  GL_CHECK( CALL_GL( gl_BufferData, GL_ELEMENT_ARRAY_BUFFER,
                     indices.size() * sizeof( IndexType ),
                     indices.data(), GL_STATIC_DRAW ) );
}

void IndexBuffer::free_resource() {
  ObjId ibo_id = resource();
  DCHECK( ibo_id != 0 );
  GL_CHECK( CALL_GL( gl_DeleteBuffers, 1, &ibo_id ) );
}

ObjId IndexBuffer::current_bound() {
  GLint id;
  GL_CHECK( CALL_GL( gl_GetIntegerv,
                     GL_ELEMENT_ARRAY_BUFFER_BINDING, &id ) );
  return (ObjId)id;
}

void IndexBuffer::bind_obj_id( ObjId new_id ) {
  GL_CHECK( CALL_GL( gl_BindBuffer, GL_ELEMENT_ARRAY_BUFFER,
                     new_id ) );
}

} // namespace gl
