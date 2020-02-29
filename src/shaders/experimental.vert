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

layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec4 in_color;
//layout (location = 1) in vec2 in_tx_coord;

uniform vec2 screen_size;

//out vec2 tx_coord;
out vec4 color;

// Convert a coordinate in screen coordinates (with 0,0 at the
// upper left) to normalized device coordinates (-1, 1) with
// (-1,-1) at the bottom right and (0,0) at the center.
vec2 to_ndc( in vec2 screen_pos ) {
  vec2 ndc_pos = screen_pos / screen_size;
  ndc_pos = ndc_pos*2.0 - vec2( 1.0 );
  ndc_pos.y = -ndc_pos.y;
  return ndc_pos;
}

void main() {
  gl_Position = vec4( to_ndc( in_pos ), 0.0, 1.0 );
  //tx_coord = in_tx_coord;
  vec4 c = in_color;
  color = c / 255.0;
}
