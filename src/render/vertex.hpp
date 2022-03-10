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
** VertexBase
*****************************************************************/
// Don't use this on its own, it is just a base class.
struct VertexBase : protected GenericVertex {
  VertexBase( GenericVertex gvert ) : GenericVertex( gvert ) {}
  GenericVertex const& generic() const { return *this; }

  // *** Depixelation.
  // Restores the shape to a default (non-depixelated) state.
  void reset_depixlation_state();
  // percent is in [0, 1.0] where 0 means totally visible and 1.0
  // means totally invisible.
  void   set_depixlation_state( double percent );
  double depixlation_state() const;

  // *** Visibility.
  bool is_visible() const;
  void set_visible( bool visible );

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
** FontVertex
*****************************************************************/
// This is a vertex used for shapes that are filled with a font
// copied from the texture atlas.
struct FontVertex : public VertexBase {
  FontVertex( gfx::point position, gfx::point atlas_position,
              gfx::pixel color );

  bool operator==( FontVertex const& ) const = default;
};

STATIC_VERTEX_CHECKS( FontVertex );

/****************************************************************
** Helpers
*****************************************************************/
template<typename VertexType>
VertexType& install_vertex( GenericVertex&    gvert,
                            VertexType const& vert ) {
  gvert = vert.generic();
  // Try to prevent UB by passing through char* which can alias
  // anything.
  char* p = reinterpret_cast<char*>( &gvert );
  return reinterpret_cast<VertexType&>( *p );
}

// WARNING: the reference returned here will only remain valid
// until the next vertex is pushed!
template<typename VertexType>
VertexType& add_vertex( std::vector<GenericVertex>& vec,
                        VertexType const&           vert ) {
  GenericVertex& gvert = vec.emplace_back();
  return install_vertex( gvert, vert );
}

} // namespace rr
