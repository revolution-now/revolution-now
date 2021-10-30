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
** VertexBufferNonTyped
*****************************************************************/
struct VertexBufferNonTyped
  : base::zero<VertexBufferNonTyped, ObjId>,
    bindable<VertexBufferNonTyped> {
  VertexBufferNonTyped();

protected:
  void upload_data_replace_impl( void const* data, size_t size,
                                 e_draw_mode mode ) const;
  void upload_data_modify_impl(
      void const* data, size_t size,
      size_t start_offset_bytes ) const;

private:
  VertexBufferNonTyped( ObjId vbo_id );

private:
  // Implement base::zero.
  friend base::zero<VertexBufferNonTyped, ObjId>;

  void free_resource();

private:
  // Implement bindable.
  friend bindable<VertexBufferNonTyped>;

  ObjId bindable_obj_id() const { return resource(); }

  static ObjId current_bound();

  static void bind_obj_id( ObjId id );
};

/****************************************************************
** VertexBuffer
*****************************************************************/
template<typename VertexType>
struct VertexBuffer : VertexBufferNonTyped {
  using vertex_type = VertexType;

  void upload_data_replace( std::span<VertexType const> data,
                            e_draw_mode mode ) const {
    upload_data_replace_impl(
        data.data(), data.size() * sizeof( VertexType ), mode );
  }

  void upload_data_modify( std::span<VertexType const> data,
                           int start_idx ) const {
    upload_data_modify_impl( data.data(),
                             data.size() * sizeof( VertexType ),
                             start_idx * sizeof( VertexType ) );
  }
};

} // namespace gl
