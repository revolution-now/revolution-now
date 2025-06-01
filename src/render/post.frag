/****************************************************************
**post.frag
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-28.
*
* Description: Fragment shader used in post processing.
*
*****************************************************************/
#version 330 core

in vec4 gl_FragCoord;

in vec2 frag_position;

uniform sampler2D u_source;
uniform vec2 u_screen_size;

out vec4 final_color;

/****************************************************************
** Helpers.
*****************************************************************/
vec4 source_lookup( in vec2 pos ) {
  pos /= u_screen_size;
  vec4 color = texture( u_source, pos );
  // We never want to sample the alpha from the source texture,
  // since this this shader phase is just copying it over.
  color.a = 1.0;
  return color;
}

/****************************************************************
** Sprites.
*****************************************************************/
// This is kind of cool, may come in handy in the future.
vec4 bright_white_blur() {
  return
    source_lookup( frag_position+vec2( 1, 0 ) ) +
    source_lookup( frag_position-vec2( 1, 0 ) ) +
    source_lookup( frag_position+vec2( 0, 1 ) ) +
    source_lookup( frag_position-vec2( 0, 1 ) )
    ;
}

vec4 use_me() {
  vec4 dummy = vec4( 0 )
    // List variables here to "use".
    ;
  return .000000000000000000000001*dummy;
}

/****************************************************************
** main
*****************************************************************/
void main() {
  // Default color red to catch errors.
  vec4 color = vec4( 1, 0, 0, 1 );

  color = source_lookup( frag_position );

  final_color = color + use_me();
}
