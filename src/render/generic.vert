/****************************************************************
**generic.vert
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-08.
*
* Description: Generic vertex shader for 2D rendering engine.
*
*****************************************************************/
#version 330 core

layout (location = 0)  in int   in_type;
layout (location = 1)  in uint  in_flags;
layout (location = 2)  in uint  in_aux_bits_1;
layout (location = 3)  in vec4  in_depixelate;
layout (location = 4)  in vec4  in_depixelate_stages;
layout (location = 5)  in vec2  in_position;
layout (location = 6)  in vec2  in_atlas_position;
layout (location = 7)  in vec4  in_atlas_rect;
layout (location = 8)  in vec2  in_reference_position_1;
layout (location = 9)  in vec2  in_reference_position_2;
layout (location = 10) in vec4  in_stencil_key_color;
layout (location = 11) in vec4  in_fixed_color;
layout (location = 12) in float in_alpha_multiplier;
layout (location = 13) in float in_scaling;
layout (location = 14) in vec2  in_translation1;
layout (location = 15) in vec2  in_translation2;

flat out int   frag_type;
flat out int   frag_aux_idx;
flat out int   frag_color_cycle_plan;
flat out int   frag_downsample;
flat out int   frag_color_cycle;
flat out int   frag_desaturate;
flat out int   frag_use_fixed_color;
flat out int   frag_uniform_depixelation;
flat out float frag_depixelate_stage;
flat out float frag_depixelate_inverted;
flat out vec2  frag_depixelate_anchor;
flat out vec4  frag_depixelate_stages;
flat out vec4  frag_depixelate_stages_unscaled;
     out vec2  frag_position;
     out vec2  frag_atlas_position;
flat out vec4  frag_atlas_rect;
flat out vec2  frag_reference_position_1;
flat out vec2  frag_reference_position_2;
     out vec4  frag_stencil_key_color;
     out vec4  frag_fixed_color;
     out float frag_alpha_multiplier;
flat out float frag_scaling;
flat out vec2  frag_default_hash_anchor;

// Screen dimensions in the game's logical pixel units.
uniform vec2  u_screen_size;
uniform vec2  u_camera_translation;
uniform float u_camera_zoom;

/****************************************************************
** Flag bit masks.
*****************************************************************/
// These need to be kept in sync with the corresponding ones in
// the C++ code.
#define VERTEX_FLAG_COLOR_CYCLE          ( uint(1) << 0 )
#define VERTEX_FLAG_USE_CAMERA           ( uint(1) << 1 )
#define VERTEX_FLAG_DESATURATE           ( uint(1) << 2 )
#define VERTEX_FLAG_FIXED_COLOR          ( uint(1) << 3 )
#define VERTEX_FLAG_UNIFORM_DEPIXELATION ( uint(1) << 4 )

int get_flag( in uint mask ) {
  uint res = in_flags & mask;
  return (res != uint(0)) ? 1 : 0;
}

bool use_camera() {
  return get_flag( VERTEX_FLAG_USE_CAMERA ) == 1;
}

/****************************************************************
** aux_bits_1 bit masks.
*****************************************************************/
// These need to be kept in sync with the corresponding ones in
// the C++ code.
#define VERTEX_AUX_BITS_1_COLOR_CYCLE ( uint(0xf) << 0 )
#define VERTEX_AUX_BITS_1_DOWNSAMPLE  ( uint(0x7) << 4 )

uint get_color_cycle() {
  return (in_aux_bits_1 & VERTEX_AUX_BITS_1_COLOR_CYCLE) >> 0;
}

uint get_downsample() {
  return (in_aux_bits_1 & VERTEX_AUX_BITS_1_DOWNSAMPLE) >> 4;
}

/****************************************************************
** Helpers.
*****************************************************************/
// Any input that refers to screen (game) coordinates needs to be
// adjusted by this function.
vec2 shift_and_scale( in vec2 position ) {
  vec2 adjusted_position = position;
  adjusted_position += in_translation1;
  adjusted_position *= in_scaling;
  adjusted_position += in_translation2;
  if( use_camera() ) {
    adjusted_position *= u_camera_zoom;
    adjusted_position += u_camera_translation;
  }
  return adjusted_position;
}

// This is for things that have units of 1/delta(logical_pixels),
// so then don't need translation but then need to be inverse
// scaled.
vec2 inverse_scale( in vec2 v ) {
  vec2 adjusted_v = v;
  adjusted_v /= in_scaling;
  if( use_camera() )
    adjusted_v /= u_camera_zoom;
  return adjusted_v;
}

// Forward things to the fragment shader that need to be for-
// warded as they are.  But note any input that refers to the
// screen position of something must be scaled/translated.
void forwarding() {
  frag_type                 = in_type;
  frag_color_cycle_plan     = int( get_color_cycle() );
  frag_downsample           = int( get_downsample() );
  frag_color_cycle          = get_flag( VERTEX_FLAG_COLOR_CYCLE );
  frag_desaturate           = get_flag( VERTEX_FLAG_DESATURATE );
  frag_use_fixed_color      = get_flag( VERTEX_FLAG_FIXED_COLOR );
  frag_uniform_depixelation = get_flag( VERTEX_FLAG_UNIFORM_DEPIXELATION );
  frag_depixelate_stage     = in_depixelate.z;
  frag_depixelate_inverted  = in_depixelate.w;
  frag_depixelate_anchor.xy = shift_and_scale( in_depixelate.xy );
  frag_depixelate_stages.zw = inverse_scale( in_depixelate_stages.zw );
  frag_depixelate_stages.xy = shift_and_scale( in_depixelate_stages.xy );
  // In the fragment shader there is a place where we need to use
  // the unscaled depixelation stage gradient, and so instead of
  // unscaling the scaled one (that we just scaled above) we will
  // just pass the original through since it avoids rounding er-
  // rors and associated visual artifacts when zoomed in.
  frag_depixelate_stages_unscaled = in_depixelate_stages;
  frag_position             = shift_and_scale( in_position );
  frag_atlas_position       = in_atlas_position;
  frag_atlas_rect           = in_atlas_rect;
  frag_reference_position_1 = in_reference_position_1;
  frag_reference_position_2 = in_reference_position_2;
  frag_stencil_key_color    = in_stencil_key_color;
  frag_fixed_color          = in_fixed_color;
  frag_alpha_multiplier     = in_alpha_multiplier;
  frag_scaling              = in_scaling;
  frag_default_hash_anchor  = shift_and_scale( vec2( 0.0 ) );
  if( use_camera() )
    frag_scaling *= u_camera_zoom;
}

// Convert a coordinate in game coordinates (meaning that 0,0 is
// at the upper left and each subsequent integer corresponds to a
// logical pixel) to normalized device coordinates (-1, 1) with
// (-1,-1) at the bottom right and (0,0) at the center.
vec2 to_ndc( in vec2 game_pos ) {
  vec2 ndc_pos = game_pos.xy / u_screen_size;
  ndc_pos = ndc_pos*2.0 - vec2( 1.0 );
  ndc_pos.y = -ndc_pos.y;
  return ndc_pos;
}

/****************************************************************
** Main.
*****************************************************************/
void main() {
  forwarding();

  vec2 adjusted_position = shift_and_scale( in_position );

  gl_Position = vec4( to_ndc( adjusted_position ), 0.0, 1.0 );
}
