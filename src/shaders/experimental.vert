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

void main() {
  gl_Position = vec4( pos, 1.0 );
}