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
layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 in_tx_coord;

uniform vec2 screen_size;

out vec4 frag_color;
out vec2 tx_coord;

vec3 to_ndc( vec3 screen_pos ) {
  vec2 ndc_pos = screen_pos.xy / screen_size;
  ndc_pos = ndc_pos*2.0 - vec2( 1.0 );
  ndc_pos.y = -ndc_pos.y;
  return vec3( ndc_pos, screen_pos.z );
}

void main() {
  gl_Position = vec4( to_ndc( pos ), 1.0 );
  frag_color = color;
  tx_coord = in_tx_coord;
}
