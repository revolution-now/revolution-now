/****************************************************************
**vertex-buffer.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-26.
*
* Description: Class to encapsulate OpenGL Vertex Buffers.
*
*****************************************************************/
#pragma once

// gl
#include "bindable.hpp"
#include "types.hpp"

// base
#include "base/zero.hpp"

// C++ standard library
#include <span>

namespace gl {

enum class e_draw_mode { stat1c, dynamic };

/****************************************************************
** VertexBuffer
*****************************************************************/
struct VertexBuffer : base::zero<VertexBuffer, ObjId>,
                      bindable<VertexBuffer> {
  VertexBuffer();

  ObjId id() const { return bindable_obj_id(); }

  template<typename T>
  void upload_data_replace( std::span<T> data,
                            e_draw_mode  mode ) const {
    upload_data_replace_impl( data.data(),
                              data.size() * sizeof( T ), mode );
  }

  template<typename T>
  void upload_data_modify( std::span<T> data,
                           int          start_idx ) const {
    upload_data_modify_impl( data.data(),
                             data.size() * sizeof( T ),
                             start_idx * sizeof( T ) );
  }

private:
  VertexBuffer( ObjId vbo_id );

  void upload_data_replace_impl( void* data, size_t size,
                                 e_draw_mode mode ) const;
  void upload_data_modify_impl(
      void* data, size_t size, size_t start_offset_bytes ) const;

private:
  // Implement base::zero.
  friend base::zero<VertexBuffer, ObjId>;

  void free_resource();

private:
  // Implement bindable.
  friend bindable<VertexBuffer>;

  ObjId bindable_obj_id() const { return resource(); }

  static ObjId current_bound();

  static void bind_obj_id( ObjId id );
};

} // namespace gl
