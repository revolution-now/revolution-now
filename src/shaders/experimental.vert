/****************************************************************
**experimental.vert
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-02-20.
*
* Description: For use when learning about shaders.
*
*****************************************************************/
#version 330 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_tx_coord;

uniform vec2 screen_size;

out vec2 tx_coord;

// Convert a coordinate in screen coordinates (with 0,0 at the
// upper left) to normalized device coordinates (-1, 1) with
// (-1,-1) at the bottom right and (0,0) at the center.
vec3 to_ndc( in vec3 screen_pos ) {
  vec2 ndc_pos = screen_pos.xy / screen_size;
  ndc_pos = ndc_pos*2.0 - vec2( 1.0 );
  ndc_pos.y = -ndc_pos.y;
  return vec3( ndc_pos, screen_pos.z );
}

void main() {
  gl_Position = vec4( to_ndc( in_pos ), 1.0 );
  tx_coord = in_tx_coord;
}
