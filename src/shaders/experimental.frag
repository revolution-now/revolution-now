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

//in vec2 io_tx_coord;
in vec4 io_color;

//uniform sampler2D u_tx;

out vec4 out_color;

void main() {
  out_color = io_color; //texture( u_tx, io_tx_coord );
}
