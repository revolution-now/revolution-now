/****************************************************************
**index-buffer.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-05.
*
* Description: RAII type for holding OpenGL index buffers.
*
*****************************************************************/
#pragma once

// gl
#include "bindable.hpp"
#include "types.hpp"

// base
#include "base/zero.hpp"

namespace gl {

/****************************************************************
** IndexBuffer
*****************************************************************/
// Currently this index buffer is just hard-coded to create a
// buffer of indices that draws triangles from quads.
struct IndexBuffer : base::zero<IndexBuffer, ObjId>,
                     bindable<IndexBuffer> {
  IndexBuffer();

 private:
  IndexBuffer( ObjId vbo_id );

 private:
  // Implement base::zero.
  friend base::zero<IndexBuffer, ObjId>;

  void free_resource();

 private:
  // Implement bindable.
  friend bindable<IndexBuffer>;

  ObjId bindable_obj_id() const { return resource(); }

  static ObjId current_bound();

  static void bind_obj_id( ObjId id );
};

} // namespace gl
