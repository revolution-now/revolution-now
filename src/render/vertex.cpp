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

using ::base::maybe;
using ::base::nothing;

enum class vertex_type {
  sprite     = 0,
  solid      = 1,
  silhouette = 2,
  stencil    = 3,
};

GenericVertex proto_vertex( vertex_type type,
                            gfx::point  position ) {
  return GenericVertex{
      .type                = static_cast<int32_t>( type ),
      .visible             = 1,
      .depixelate          = gl::vec4{},
      .depixelate_stages   = gl::vec4{},
      .position            = gl::vec2::from_point( position ),
      .atlas_position      = {},
      .atlas_center        = {},
      .atlas_target_offset = {},
      .fixed_color         = {},
      .alpha_multiplier    = 1.0f,
      .scaling             = 1.0,
      .translation         = {},
      .color_cycle         = 0, // "false"
      .use_camera          = 0, // "false"
  };
}

} // namespace

/****************************************************************
** VertexBase
*****************************************************************/
void VertexBase::set_depixelation_stage( double percent ) {
  depixelate.z = static_cast<float>( percent );
}

double VertexBase::depixelation_stage() const {
  return depixelate.z;
}

void VertexBase::set_depixelation_hash_anchor(
    gfx::point anchor ) {
  depixelate.x = static_cast<float>( anchor.x );
  depixelate.y = static_cast<float>( anchor.y );
}

gl::vec2 VertexBase::depixelation_hash_anchor() const {
  return gl::vec2{ .x = depixelate.x, .y = depixelate.y };
}

void VertexBase::set_depixelation_gradient(
    gfx::dsize gradient ) {
  // Note the w's are not wrongly swapped.
  depixelate_stages.z = gradient.w;
  depixelate_stages.w = gradient.h;
}

gl::vec2 VertexBase::depixelation_gradient() const {
  return gl::vec2{ .x = depixelate_stages.z,
                   .y = depixelate_stages.w };
}

void VertexBase::set_depixelation_stage_anchor(
    gfx::dpoint anchor ) {
  depixelate_stages.x = anchor.x;
  depixelate_stages.y = anchor.y;
}

gl::vec2 VertexBase::depixelation_stage_anchor() const {
  return gl::vec2{ .x = depixelate_stages.x,
                   .y = depixelate_stages.y };
}

void VertexBase::set_depixelation_inversion( bool inverted ) {
  depixelate.w = inverted ? 1 : 0;
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

void VertexBase::set_scaling( double scale ) {
  scaling = static_cast<float>( scale );
}

void VertexBase::set_translation( gfx::dsize trans ) {
  translation = gl::vec2::from_dsize( trans );
}

void VertexBase::set_color_cycle( bool enabled ) {
  color_cycle = enabled ? 1 : 0;
}

void VertexBase::set_use_camera( bool enabled ) {
  use_camera = enabled ? 1 : 0;
}

/****************************************************************
** SpriteVertex
*****************************************************************/
SpriteVertex::SpriteVertex( gfx::point position,
                            gfx::point atlas_position,
                            gfx::point atlas_center )
  : VertexBase( proto_vertex( vertex_type::sprite, position ) ) {
  this->atlas_position = gl::vec2::from_point( atlas_position );
  this->atlas_center   = gl::vec2::from_point( atlas_center );
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
                                    gfx::point atlas_center,
                                    gfx::pixel color )
  : VertexBase(
        proto_vertex( vertex_type::silhouette, position ) ) {
  this->atlas_position = gl::vec2::from_point( atlas_position );
  this->atlas_center   = gl::vec2::from_point( atlas_center );
  this->fixed_color    = gl::color::from_pixel( color );
}

/****************************************************************
** StencilVertex
*****************************************************************/
StencilVertex::StencilVertex( gfx::point position,
                              gfx::point atlas_position,
                              gfx::point atlas_center,
                              gfx::size  atlas_target_offset,
                              gfx::pixel key_color )
  : VertexBase(
        proto_vertex( vertex_type::stencil, position ) ) {
  this->atlas_position = gl::vec2::from_point( atlas_position );
  this->atlas_center   = gl::vec2::from_point( atlas_center );
  this->atlas_target_offset =
      gl::vec2::from_size( atlas_target_offset );
  this->fixed_color = gl::color::from_pixel( key_color );
}

} // namespace rr
