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
  sprite  = 0,
  solid   = 1,
  stencil = 2,
};

GenericVertex proto_vertex( vertex_type type,
                            gfx::point  position ) {
  return GenericVertex{
      .type                = static_cast<int32_t>( type ),
      .flags               = 0,
      .depixelate          = gl::vec4{},
      .depixelate_stages   = gl::vec4{},
      .position            = gl::vec2::from_point( position ),
      .atlas_position      = {},
      .atlas_rect          = {},
      .atlas_target_offset = {},
      .fixed_color         = {},
      .alpha_multiplier    = 1.0f,
      .scaling             = 1.0,
      .translation         = {},
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
  auto constexpr mask = VERTEX_FLAG_COLOR_CYCLE;
  if( enabled )
    flags |= mask;
  else
    flags &= ~mask;
}

bool VertexBase::get_color_cycle() const {
  auto constexpr mask = VERTEX_FLAG_COLOR_CYCLE;
  return ( ( flags & mask ) != 0 ) ? true : false;
}

void VertexBase::set_use_camera( bool enabled ) {
  auto constexpr mask = VERTEX_FLAG_USE_CAMERA;
  if( enabled )
    flags |= mask;
  else
    flags &= ~mask;
}

bool VertexBase::get_use_camera() const {
  auto constexpr mask = VERTEX_FLAG_USE_CAMERA;
  return ( ( flags & mask ) != 0 ) ? true : false;
}

void VertexBase::set_desaturate( bool enabled ) {
  auto constexpr mask = VERTEX_FLAG_DESATURATE;
  if( enabled )
    flags |= mask;
  else
    flags &= ~mask;
}

bool VertexBase::get_desaturate() const {
  auto constexpr mask = VERTEX_FLAG_DESATURATE;
  return ( ( flags & mask ) != 0 ) ? true : false;
}

void VertexBase::set_fixed_color(
    base::maybe<gfx::pixel> color ) {
  auto constexpr mask = VERTEX_FLAG_FIXED_COLOR;
  if( !color.has_value() ) {
    flags &= ~mask;
    fixed_color = {};
    return;
  }
  flags |= mask;
  fixed_color.r = color->r / 255.0;
  fixed_color.g = color->g / 255.0;
  fixed_color.b = color->b / 255.0;
  fixed_color.a = color->a / 255.0;
}

base::maybe<gfx::pixel> VertexBase::get_fixed_color() const {
  if( !( flags & VERTEX_FLAG_FIXED_COLOR ) ) return nothing;
  auto to_8_bit = []( float f ) {
    return static_cast<uint8_t>(
        clamp( static_cast<int>( floor( f * 255 ) ), 0, 255 ) );
  };
  return gfx::pixel{ .r = to_8_bit( fixed_color.r ),
                     .g = to_8_bit( fixed_color.g ),
                     .b = to_8_bit( fixed_color.b ),
                     .a = to_8_bit( fixed_color.a ) };
}

void VertexBase::set_uniform_depixelation( bool enabled ) {
  auto constexpr mask = VERTEX_FLAG_UNIFORM_DEPIXELATION;
  if( enabled )
    flags |= mask;
  else
    flags &= ~mask;
}

bool VertexBase::get_uniform_depixelation() const {
  auto constexpr mask = VERTEX_FLAG_UNIFORM_DEPIXELATION;
  return ( ( flags & mask ) != 0 ) ? true : false;
}

/****************************************************************
** SpriteVertex
*****************************************************************/
SpriteVertex::SpriteVertex( gfx::point position,
                            gfx::point atlas_position,
                            gfx::rect  atlas_rect )
  : VertexBase( proto_vertex( vertex_type::sprite, position ) ) {
  this->atlas_position = gl::vec2::from_point( atlas_position );
  this->atlas_rect     = gl::vec4::from_rect( atlas_rect );
}

/****************************************************************
** SolidVertex
*****************************************************************/
SolidVertex::SolidVertex( gfx::point position, gfx::pixel color )
  : VertexBase( proto_vertex( vertex_type::solid, position ) ) {
  this->fixed_color = gl::color::from_pixel( color );
}

/****************************************************************
** StencilVertex
*****************************************************************/
StencilVertex::StencilVertex( gfx::point position,
                              gfx::point atlas_position,
                              gfx::rect  atlas_rect,
                              gfx::size  atlas_target_offset,
                              gfx::pixel key_color )
  : VertexBase(
        proto_vertex( vertex_type::stencil, position ) ) {
  this->atlas_position = gl::vec2::from_point( atlas_position );
  this->atlas_rect     = gl::vec4::from_rect( atlas_rect );
  this->atlas_target_offset =
      gl::vec2::from_size( atlas_target_offset );
  this->stencil_key_color = gl::color::from_pixel( key_color );
}

} // namespace rr
