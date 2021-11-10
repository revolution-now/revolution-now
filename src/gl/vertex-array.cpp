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
#include "iface.hpp"

// base
#include "base/error.hpp"
#include "base/fmt.hpp"

using namespace std;

namespace gl {

/****************************************************************
** VertexArrayNonTyped
*****************************************************************/
VertexArrayNonTyped::VertexArrayNonTyped() {
  ObjId vao_id = 0;
  GL_CHECK( CALL_GL( gl_GenVertexArrays, 1, &vao_id ) );
  *this = VertexArrayNonTyped( vao_id );
}

VertexArrayNonTyped::VertexArrayNonTyped( ObjId vao_id )
  : base::zero<VertexArrayNonTyped, ObjId>( vao_id ),
    bindable<VertexArrayNonTyped>() {}

void VertexArrayNonTyped::free_resource() {
  ObjId vao_id = resource();
  DCHECK( vao_id != 0 );
  GL_CHECK( CALL_GL( gl_DeleteVertexArrays, 1, &vao_id ) );
}

ObjId VertexArrayNonTyped::current_bound() {
  GLint id;
  GL_CHECK(
      CALL_GL( gl_GetIntegerv, GL_VERTEX_ARRAY_BINDING, &id ) );
  return (ObjId)id;
}

void VertexArrayNonTyped::bind_obj_id( ObjId new_id ) {
  GL_CHECK( CALL_GL( gl_BindVertexArray, new_id ) );
}

void VertexArrayNonTyped::register_attrib(
    int idx, size_t attrib_field_count,
    e_attrib_type component_type, bool normalized, size_t stride,
    size_t offset ) const {
  static int kMaxAttributesAllowed = [] {
    int n;
    glGetIntegerv( GL_MAX_VERTEX_ATTRIBS, &n );
    return n;
  }();
  CHECK_LT( idx, kMaxAttributesAllowed,
            "graphics card/driver only supports up to {} vertex "
            "attributes.",
            kMaxAttributesAllowed );
  GLboolean gl_normalized = normalized ? GL_TRUE : GL_FALSE;
#if 1
  fmt::print(
      "registering vertex attribute: {{idx={}, "
      "attrib_field_count={}, component_type={}, normalized={}, "
      "stride={}, offset={}}}\n",
      idx, attrib_field_count, to_GL_str( component_type ),
      normalized, stride, offset );
#endif
  GL_CHECK( CALL_GL( gl_VertexAttribPointer, idx,
                     attrib_field_count, to_GL( component_type ),
                     gl_normalized, stride, (void*)offset ) );
  GL_CHECK( CALL_GL( gl_EnableVertexAttribArray, idx ) );
}

} // namespace gl
