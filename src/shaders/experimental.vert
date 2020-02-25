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

out vec4 frag_color;
out vec2 tx_coord;

void main() {
  gl_Position = vec4( pos, 1.0 );
  frag_color = color;
  tx_coord = in_tx_coord;
}
