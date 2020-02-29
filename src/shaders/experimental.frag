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

//in vec2 tx_coord;
in vec4 color;

//uniform sampler2D tx;

out vec4 frag_color;

void main() {
  frag_color = color; //texture( tx, tx_coord );
}
