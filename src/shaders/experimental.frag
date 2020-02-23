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

in  vec3 frag_color;
out vec4 FragColor;

void main() {
  FragColor = vec4( frag_color, 1.0f );
}
