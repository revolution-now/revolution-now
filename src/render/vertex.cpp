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

// C++ standard library
#include <cmath>

using namespace std;

namespace rr {

namespace {

using ::base::maybe;
using ::base::nothing;
using ::gfx::pixel;
using ::gfx::point;

enum class vertex_type {
  sprite  = 0,
  solid   = 1,
  stencil = 2,
  line    = 3,
};

GenericVertex proto_vertex( vertex_type type,
                            gfx::point position ) {
  return GenericVertex{
    .type                 = static_cast<int32_t>( type ),
    .flags                = 0,
    .aux_bits_1           = 0,
    .depixelate           = gl::vec4{},
    .depixelate_stages    = gl::vec4{},
    .position             = gl::vec2::from_point( position ),
    .atlas_position       = {},
    .atlas_rect           = {},
    .reference_position_1 = {},
    .fixed_color          = {},
    .alpha_multiplier     = 1.0f,
    .scaling              = 1.0,
    .translation1         = {},
    .translation2         = {},
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

void VertexBase::set_translation1( gfx::dsize trans ) {
  translation1 = gl::vec2::from_dsize( trans );
}

void VertexBase::set_translation2( gfx::dsize trans ) {
  translation2 = gl::vec2::from_dsize( trans );
}

uint32_t VertexBase::get_color_cycle_plan() const {
  static auto constexpr kMask  = VERTEX_AUX_BITS_1_COLOR_CYCLE;
  static auto constexpr kShift = countr_zero( kMask );
  return ( aux_bits_1 & kMask ) >> kShift;
}

void VertexBase::set_color_cycle_plan( uint32_t const arr ) {
  static auto constexpr kMask  = VERTEX_AUX_BITS_1_COLOR_CYCLE;
  static auto constexpr kShift = countr_zero( kMask );
  aux_bits_1 &= ~kMask;
  aux_bits_1 |= ( arr & ( kMask >> kShift ) ) << kShift;
}

uint32_t VertexBase::get_downsampling_power() const {
  static auto constexpr kMask  = VERTEX_AUX_BITS_1_DOWNSAMPLE;
  static auto constexpr kShift = countr_zero( kMask );
  return ( aux_bits_1 & kMask ) >> kShift;
}

void VertexBase::set_downsampling_power( uint32_t const arr ) {
  static auto constexpr kMask  = VERTEX_AUX_BITS_1_DOWNSAMPLE;
  static auto constexpr kShift = countr_zero( kMask );
  aux_bits_1 &= ~kMask;
  aux_bits_1 |= ( arr & ( kMask >> kShift ) ) << kShift;
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

void VertexBase::set_textured_depixelation(
    maybe<TxDpxl> const txdpxl ) {
  auto constexpr mask = VERTEX_FLAG_TEXTURED_DEPIXELATION;
  if( txdpxl ) {
    flags |= mask;
    reference_position_2.x = txdpxl->reference_sprite_offset.w;
    reference_position_2.y = txdpxl->reference_sprite_offset.h;
  } else {
    flags &= ~mask;
  }
}

maybe<TxDpxl> VertexBase::get_textured_depixelation() const {
  auto constexpr mask = VERTEX_FLAG_TEXTURED_DEPIXELATION;
  bool const enabled  = ( ( flags & mask ) != 0 ) ? true : false;
  if( !enabled ) return nothing;
  return TxDpxl{
    .reference_sprite_offset = {
      .w = static_cast<int>( reference_position_2.x ),
      .h = static_cast<int>( reference_position_2.y ),
    } };
}

/****************************************************************
** SpriteVertex
*****************************************************************/
SpriteVertex::SpriteVertex( gfx::point position,
                            gfx::point atlas_position,
                            gfx::rect atlas_rect,
                            maybe<TxDpxl> const txdpxl )
  : VertexBase( proto_vertex( vertex_type::sprite, position ) ) {
  this->atlas_position = gl::vec2::from_point( atlas_position );
  this->atlas_rect     = gl::vec4::from_rect( atlas_rect );
  set_textured_depixelation( txdpxl );
}

/****************************************************************
** SolidVertex
*****************************************************************/
SolidVertex::SolidVertex( gfx::point position, gfx::pixel color )
  : VertexBase( proto_vertex( vertex_type::solid, position ) ) {
  this->fixed_color = gl::color::from_pixel( color );
}

/****************************************************************
** SpriteStencilVertex
*****************************************************************/
SpriteStencilVertex::SpriteStencilVertex(
    gfx::point position, gfx::point atlas_position,
    gfx::rect atlas_rect, gfx::size atlas_target_offset,
    gfx::pixel key_color, maybe<TxDpxl> const txdpxl )
  : VertexBase(
        proto_vertex( vertex_type::stencil, position ) ) {
  this->atlas_position = gl::vec2::from_point( atlas_position );
  this->atlas_rect     = gl::vec4::from_rect( atlas_rect );
  this->reference_position_1 =
      gl::vec2::from_size( atlas_target_offset );
  this->stencil_key_color = gl::color::from_pixel( key_color );
  set_textured_depixelation( txdpxl );
}

/****************************************************************
** LineVertex
*****************************************************************/
LineVertex::LineVertex( point const position,
                        point const line_start,
                        point const line_end, pixel const color )
  : VertexBase( proto_vertex( vertex_type::line, position ) ) {
  this->fixed_color = gl::color::from_pixel( color );
  this->reference_position_1 =
      gl::vec2::from_point( line_start );
  this->reference_position_2 = gl::vec2::from_point( line_end );
}

} // namespace rr
