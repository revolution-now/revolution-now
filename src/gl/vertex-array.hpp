/****************************************************************
**vertex-array.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-27.
*
* Description: Class to encapsulate OpenGL Vertex Arrays.
*
*****************************************************************/
#pragma once

// gl
#include "attribs.hpp"
#include "bindable.hpp"
#include "types.hpp"
#include "vertex-buffer.hpp"

// refl
#include "refl/ext.hpp"

// base
#include "base/meta.hpp"
#include "base/zero.hpp"

// C++ standard library
#include <cstddef>

namespace gl {

/****************************************************************
** VertexArrayNonTyped
*****************************************************************/
struct VertexArrayNonTyped
  : base::zero<VertexArrayNonTyped, ObjId>,
    bindable<VertexArrayNonTyped> {
  VertexArrayNonTyped();

  ObjId id() const { return bindable_obj_id(); }

 protected:
  // Vertex Array and Vertex Buffer must be bound before calling
  // this.
  void register_attrib( int idx, size_t attrib_field_count,
                        e_attrib_type component_type,
                        bool normalized, size_t stride,
                        size_t offset ) const;

 private:
  VertexArrayNonTyped( ObjId vbo_id );

 private:
  // Implement base::zero.
  friend base::zero<VertexArrayNonTyped, ObjId>;

  void free_resource();

 private:
  // Implement bindable.
  friend bindable<VertexArrayNonTyped>;

  ObjId bindable_obj_id() const { return resource(); }

  static ObjId current_bound();

  static void bind_obj_id( ObjId id );
};

/****************************************************************
** VertexField
*****************************************************************/
template<typename...>
struct VertexFieldTypeList;

template<typename... T>
struct VertexFieldTypeList<std::tuple<T...>> {
  using type = mp::list<typename T::type...>;
};

template<typename... T>
using VertexFieldTypeList_t =
    typename VertexFieldTypeList<T...>::type;

/****************************************************************
** VertexArray
*****************************************************************/
template<typename...>
struct VertexArray;

template<refl::ReflectedStruct... VertexTypes>
struct VertexArray<VertexBuffer<VertexTypes>...>
  : VertexArrayNonTyped {
  // This will be an mp::list of all of the attribute types, in
  // order, from all of the buffers concatenated and flattened.
  using AttribTypeList =
      VertexFieldTypeList_t<decltype( std::tuple_cat(
          refl::traits<VertexTypes>::fields... ) )>;

  VertexArray() {
    register_attribs_for_all_buffers(
        std::index_sequence_for<VertexTypes...>() );
  }

  template<size_t N>
  auto const& buffer() const {
    return std::get<N>( buffers_ );
  }

 private:
  template<size_t... Idxs>
  void register_attribs_for_all_buffers(
      std::index_sequence<Idxs...> ) const {
    auto binder           = bind();
    int  attrib_idx_start = 0;
    ( ( attrib_idx_start +=
        register_attribs_for_single_buffer<Idxs>(
            attrib_idx_start ) ),
      ... );
  }

  // One buffer has multiple attributes.
  template<size_t BufferIdx>
  int register_attribs_for_single_buffer(
      int const attrib_idx_start ) const {
    auto const& buffer        = std::get<BufferIdx>( buffers_ );
    auto        buffer_binder = buffer.bind();

    using VertexType = typename std::remove_cvref_t<
        decltype( buffer )>::vertex_type;
    static constexpr auto const& attribs =
        refl::traits<VertexType>::fields;
    static constexpr size_t kNumAttribs = std::tuple_size_v<
        std::remove_cvref_t<decltype( attribs )>>;

    FOR_CONSTEXPR_IDX( AttribIdx, kNumAttribs ) {
      constexpr auto const& attrib =
          std::get<AttribIdx>( attribs );
      using AttribType = typename std::remove_cvref_t<
          decltype( std::get<AttribIdx>( attribs ) )>::type;
      constexpr auto& offset = attrib.offset;
      // Make sure that we have an offset.
      static_assert( offset.index() == 1 );
      this->register_attrib(
          attrib_idx_start + AttribIdx,
          /*attrib_field_count=*/
          attrib_traits<AttribType>::count,
          /*component_type=*/
          attrib_traits<AttribType>::component_type,
          /*normalized=*/false,
          /*stride=*/sizeof( VertexType ),
          /*offset=*/offset.template get<size_t>() );
    };
    return kNumAttribs;
  }

  std::tuple<VertexBuffer<VertexTypes>...> buffers_;
};

} // namespace gl
