/****************************************************************
**vertex.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-07.
*
* Description: Generic Vertex type for 2D rendering engine.
*
*****************************************************************/
#include "vertex.hpp"

using namespace std;

namespace rr {

namespace {

enum class vertex_type {
  sprite     = 0,
  solid      = 1,
  silhouette = 2,
};

GenericVertex proto_vertex( vertex_type type,
                            gfx::point  position ) {
  return GenericVertex{
      .type             = static_cast<int32_t>( type ),
      .visible          = 1,
      .depixelate       = 0.0f,
      .position         = gl::vec2::from_point( position ),
      .atlas_position   = {}, // Caller maybe fills in.
      .fixed_color      = {}, // Caller maybe fills in.
      .alpha_multiplier = 1.0f,
  };
}

}

/****************************************************************
** VertexBase
*****************************************************************/
void VertexBase::reset_depixelation_state() {
  set_depixelation_state( 0.0f );
}

void VertexBase::set_depixelation_state( double percent ) {
  depixelate = static_cast<float>( percent );
}

double VertexBase::depixlation_state() const {
  return depixelate;
}

bool VertexBase::is_visible() const {
  DCHECK( visible == 0 || visible == 1 );
  return visible == 1;
}

void VertexBase::set_visible( bool in_visible ) {
  visible = in_visible ? 1 : 0;
}

double VertexBase::alpha() const { return alpha_multiplier; }

void VertexBase::reset_alpha() { alpha_multiplier = 1.0f; }

void VertexBase::set_alpha( double alpha ) {
  alpha_multiplier = static_cast<float>( alpha );
}

/****************************************************************
** SpriteVertex
*****************************************************************/
SpriteVertex::SpriteVertex( gfx::point position,
                            gfx::point atlas_position )
  : VertexBase( proto_vertex( vertex_type::sprite, position ) ) {
  this->atlas_position = gl::vec2::from_point( atlas_position );
}

/****************************************************************
** SolidVertex
*****************************************************************/
SolidVertex::SolidVertex( gfx::point position, gfx::pixel color )
  : VertexBase( proto_vertex( vertex_type::solid, position ) ) {
  this->fixed_color = gl::color::from_pixel( color );
}

/****************************************************************
** SilhouetteVertex
*****************************************************************/
SilhouetteVertex::SilhouetteVertex( gfx::point position,
                                    gfx::point atlas_position,
                                    gfx::pixel color )
  : VertexBase(
        proto_vertex( vertex_type::silhouette, position ) ) {
  this->atlas_position = gl::vec2::from_point( atlas_position );
  this->fixed_color    = gl::color::from_pixel( color );
}

} // namespace rr
