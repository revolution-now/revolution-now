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

in  vec4 frag_color;
in  vec2 tx_coord;

uniform sampler2D tx;

out vec4 FragColor;

void main() {
  if( frag_color == vec4( 0.0 ) )
    FragColor = texture( tx, tx_coord );
  else
    FragColor = frag_color;
}
