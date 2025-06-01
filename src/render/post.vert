/****************************************************************
**post.vert
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-28.
*
* Description: Vertex shader used in post processing.
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

out vec2 frag_position;

uniform vec2 u_screen_size;

/****************************************************************
** Helpers.
*****************************************************************/
void forward() {
  frag_position = in_position;
}

// Convert a coordinate in game coordinates (meaning that 0,0 is
// at the upper left and each subsequent integer corresponds to a
// logical pixel) to normalized device coordinates (-1, 1) with
// (-1,-1) at the bottom right and (0,0) at the center.
vec2 to_ndc( in vec2 game_pos ) {
  vec2 ndc_pos = game_pos.xy / u_screen_size;
  ndc_pos = ndc_pos*2.0 - vec2( 1.0 );
  // NOTE: No flipping of y here.
  return ndc_pos;
}

vec4 use_me() {
  vec4 dummy = vec4( 0 )
    // List variables here to "use".
    + vec4( in_type )
    + vec4( in_flags )
    + vec4( in_aux_bits_1 )
    + vec4( in_depixelate )
    + vec4( in_depixelate_stages )
    + vec4( in_position, 0, 0 )
    + vec4( in_atlas_position, 0, 0 )
    + vec4( in_atlas_rect )
    + vec4( in_reference_position_1, 0, 0 )
    + vec4( in_reference_position_2, 0, 0 )
    + vec4( in_stencil_key_color )
    + vec4( in_fixed_color )
    + vec4( in_alpha_multiplier )
    + vec4( in_scaling )
    + vec4( in_translation1, 0, 0 )
    + vec4( in_translation2, 0, 0 )
    ;
  return .000000000000000000000001*dummy;
}

/****************************************************************
** Main.
*****************************************************************/
void main() {
  forward();

  gl_Position = vec4( to_ndc( in_position ), 0.0, 1.0 ) + use_me();
}
