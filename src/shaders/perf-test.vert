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
layout (location = 1) in vec2 in_tx_coord;
layout (location = 2) in vec2 in_center;

uniform vec2 screen_size;
uniform vec2 tick;

out vec2 tx_coord;

vec2 rotate( vec2 point, float degree, vec2 pivot )
{
  float rad_angle = -radians( degree ); // "-" for clockwise
  float x = point.x;
  float y = point.y;

  float rX = pivot.x + (x - pivot.x) * cos( rad_angle )
                     - (y - pivot.y) * sin( rad_angle );
  float rY = pivot.y + (x - pivot.x) * sin( rad_angle )
                     + (y - pivot.y) * cos( rad_angle );

  return vec2( rX, rY );
}

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
  vec2 new_pos = in_pos;
  new_pos = rotate( new_pos, tick.x/45.0, in_center );
  new_pos = new_pos + floor( 100*sin( tick.x/2000 ) );
  gl_Position = vec4( to_ndc( vec3( new_pos, 1.0 ) ), 1.0 );
  tx_coord = in_tx_coord;
}
