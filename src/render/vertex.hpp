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
** Flag bit masks.
*****************************************************************/
// These need to be kept in sync with the corresponding ones in
// the shader.
#define VERTEX_FLAG_COLOR_CYCLE          ( uint32_t{ 1 } << 0 )
#define VERTEX_FLAG_USE_CAMERA           ( uint32_t{ 1 } << 1 )
#define VERTEX_FLAG_DESATURATE           ( uint32_t{ 1 } << 2 )
#define VERTEX_FLAG_FIXED_COLOR          ( uint32_t{ 1 } << 3 )
#define VERTEX_FLAG_UNIFORM_DEPIXELATION ( uint32_t{ 1 } << 4 )

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
  void set_depixelation_stage( double percent );
  double depixelation_stage() const;
  // Used to allow the depixelation animation to proceed deter-
  // ministically even as the sprite being depixelated moves
  // around.
  void set_depixelation_hash_anchor( gfx::point anchor );
  gl::vec2 depixelation_hash_anchor() const;
  // Specifies how the depixelation stage should vary across the
  // triangle. Each component is a slope, thus the depixelation
  // stage will vary like a 2d plane.
  void set_depixelation_gradient( gfx::dsize gradient );
  gl::vec2 depixelation_gradient() const;
  // This is the anchor point from which the depixelation stage
  // will be extrapolated if it is gradiated.
  void set_depixelation_stage_anchor( gfx::dpoint anchor );
  gl::vec2 depixelation_stage_anchor() const;
  // Flips the state of each pixel.
  void set_depixelation_inversion( bool inverted );
  bool depixelation_inversion() const;

  // *** Alpha in [0, 1].
  double alpha() const;
  void reset_alpha();
  void set_alpha( double alpha );

  // *** Repositioning.
  void set_scaling( double scale );
  void set_translation( gfx::dsize translation );

  // *** Auxiliary index.
  int32_t get_aux_idx() const;
  void set_aux_idx( int32_t value );

  // *** Color Cycling.
  bool get_color_cycle() const;
  void set_color_cycle( bool enabled );

  // *** Camera.
  bool get_use_camera() const;
  void set_use_camera( bool enabled );

  // *** Desaturation.
  bool get_desaturate() const;
  void set_desaturate( bool enabled );

  // *** Fixed color.
  base::maybe<gfx::pixel> get_fixed_color() const;
  void set_fixed_color( base::maybe<gfx::pixel> color );

  // *** Uniform depixelation.
  bool get_uniform_depixelation() const;
  void set_uniform_depixelation( bool enabled );

  bool operator==( VertexBase const& ) const = default;
};

STATIC_VERTEX_CHECKS( VertexBase );

/****************************************************************
** SpriteVertex
*****************************************************************/
// This is a vertex used for shapes that are copied from the tex-
// ture atlas as a source.
struct SpriteVertex : public VertexBase {
  SpriteVertex( gfx::point position, gfx::point atlas_position,
                gfx::rect atlas_rect );

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
** StencilVertex
*****************************************************************/
// This is a vertex used for shapes that are filled with a sprite
// copied from the texture atlas but where any colors in the
// sprite matching a key color are replaced by pixels from an al-
// ternate sprite with alpha multiplication.
struct StencilVertex : public VertexBase {
  StencilVertex( gfx::point position, gfx::point atlas_position,
                 gfx::rect atlas_rect,
                 gfx::size atlas_target_offset,
                 gfx::pixel key_color );

  bool operator==( StencilVertex const& ) const = default;
};

STATIC_VERTEX_CHECKS( StencilVertex );

} // namespace rr
