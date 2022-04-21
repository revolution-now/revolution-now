/****************************************************************
**vertex.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-07.
*
* Description: Generic Vertex type for 2D rendering engine.
*
*****************************************************************/
#pragma once

// Rds
#include "vertex.rds.hpp"

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/pixel.hpp"

// C++ standard library
#include <vector>

namespace rr {

// These checks are important because we need to send arrays of
// GenericVertex into the GPU.
#define STATIC_VERTEX_CHECKS( type )                          \
  static_assert( sizeof( type ) == sizeof( GenericVertex ) ); \
  static_assert( std::alignment_of_v<type> ==                 \
                 std::alignment_of_v<GenericVertex> );

/****************************************************************
** Concept
*****************************************************************/
template<typename T>
concept VertexType = requires( T const& v ) {
  { v.generic() } -> std::same_as<GenericVertex const&>;
};

/****************************************************************
** VertexBase
*****************************************************************/
// Don't use this on its own, it is just a base class.
struct VertexBase : protected GenericVertex {
  VertexBase( GenericVertex gvert ) : GenericVertex( gvert ) {}
  GenericVertex const& generic() const { return *this; }

  // *** Depixelation.
  // Percent is in [0, 1.0] where 0 means totally visible and 1.0
  // means totally invisible.
  void   set_depixelation_stage( double percent );
  double depixelation_stage() const;
  // Used to allow the depixelation animation to proceed deter-
  // ministically even as the sprite being depixelated moves
  // around.
  void     set_depixelation_anchor( gfx::point anchor );
  gl::vec2 depixelation_anchor() const;
  // If the sprite is depixelating to a different sprite then the
  // second argument will be the offset in the texture atlas to
  // go from this vertex to the corresponding vertex on the
  // target sprite. Otherwise, if the second argument is zero,
  // then we just depixelate to nothing.
  void set_depixelation_target( gfx::size target_atlas_offset );

  // *** Visibility.
  bool is_visible() const;
  void set_visible( bool visible );

  // *** Alpha in [0, 1].
  double alpha() const;
  void   reset_alpha();
  void   set_alpha( double alpha );

  // *** Repositioning.
  void set_scaling( double scale );
  void set_translation( gfx::size translation );

  // *** Color Cycling.
  void set_color_cycle( bool enabled );

  // *** Camera.
  void set_use_camera( bool enabled );

  bool operator==( VertexBase const& ) const = default;
};

STATIC_VERTEX_CHECKS( VertexBase );

/****************************************************************
** SpriteVertex
*****************************************************************/
// This is a vertex used for shapes that are copied from the tex-
// ture atlas as a source.
struct SpriteVertex : public VertexBase {
  SpriteVertex( gfx::point position, gfx::point atlas_position );

  bool operator==( SpriteVertex const& ) const = default;
};

STATIC_VERTEX_CHECKS( SpriteVertex );

/****************************************************************
** SolidVertex
*****************************************************************/
// This is a vertex used for shapes that are filled with a solid
// color.
struct SolidVertex : public VertexBase {
  SolidVertex( gfx::point position, gfx::pixel color );

  bool operator==( SolidVertex const& ) const = default;
};

STATIC_VERTEX_CHECKS( SolidVertex );

/****************************************************************
** SilhouetteVertex
*****************************************************************/
// This is a vertex used for shapes that are filled with a sprite
// copied from the texture atlas but where the specified color is
// used for all pixels with its alpha multiplied by the alpha of
// the texture atlas pixel.
struct SilhouetteVertex : public VertexBase {
  SilhouetteVertex( gfx::point position,
                    gfx::point atlas_position,
                    gfx::pixel color );

  bool operator==( SilhouetteVertex const& ) const = default;
};

STATIC_VERTEX_CHECKS( SilhouetteVertex );

/****************************************************************
** StencilVertex
*****************************************************************/
// This is a vertex used for shapes that are filled with a sprite
// copied from the texture atlas but where any colors in the
// sprite matching a key color are replaced by pixels from an al-
// ternate sprite with alpha multiplication.
struct StencilVertex : public VertexBase {
  StencilVertex( gfx::point position, gfx::point atlas_position,
                 gfx::size  atlas_target_offset,
                 gfx::pixel key_color );

  bool operator==( StencilVertex const& ) const = default;
};

STATIC_VERTEX_CHECKS( StencilVertex );

/****************************************************************
** Helpers
*****************************************************************/
template<VertexType V>
V& vertex_cast( GenericVertex& gvert ) {
  STATIC_VERTEX_CHECKS( V );
  // Try to prevent UB by passing through char* which can alias
  // anything.
  char* p = reinterpret_cast<char*>( &gvert );
  return reinterpret_cast<V&>( *p );
}

template<VertexType V>
V& install_vertex( GenericVertex& gvert, V const& vert ) {
  gvert = vert.generic();
  return vertex_cast<V>( gvert );
}

// WARNING: the reference returned here will only remain valid
// until the next vertex is pushed!
template<VertexType V>
V& add_vertex( std::vector<GenericVertex>& vec, V const& vert ) {
  GenericVertex& gvert = vec.emplace_back();
  return install_vertex( gvert, vert );
}

} // namespace rr
