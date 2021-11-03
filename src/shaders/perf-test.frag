/****************************************************************
**experimental.frag
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-02-20.
*
* Description: For use when learning about shaders.
*
*****************************************************************/
#version 330 core

in vec2 tx_coord;
in vec3 shading_color;

uniform sampler2D tx;

out vec4 frag_color;

void main() {
  vec4 tx_color = texture( tx, tx_coord );
  vec4 shading = vec4( shading_color, 1.0 );
  vec4 net_color = (tx_color + shading)/2;
  frag_color = net_color;
}
