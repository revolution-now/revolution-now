/****************************************************************
**generic.frag
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-08.
*
* Description: Generic fragment shader for 2D rendering engine.
*
*****************************************************************/
#version 330 core

in int   frag_type;
in int   frag_depixelate;
in vec2  frag_position;
in vec2  frag_atlas_position;
in vec4  frag_fixed_color;
in float frag_alpha_multiplier;

uniform sampler2D u_atlas;

out vec4 final_color;

/****************************************************************
** Sprites.
*****************************************************************/
vec4 type_sprite() {
  return texture( u_atlas, frag_atlas_position );
}

/****************************************************************
** Solids.
*****************************************************************/
vec4 type_solid() {
  return frag_fixed_color;
}

/****************************************************************
** Silhouettes.
*****************************************************************/
vec4 type_silhouette() {
  return frag_fixed_color*vec4( 1, 1, 1, type_sprite().a );
}

/****************************************************************
** Depixelation.
*****************************************************************/
// Here we will produce a hash of the correct pixel coordinates
// (in game space, so they will be integers) and use that as a
// deterministic pseudo-random number for each pixel. Comparing
// that with the `frag_depixelation` animation state, we can ran-
// domly hide pixels with increasing probability, effectively
// creating the "depixelation" animation that happens when a unit
// or colony is destroyed or defeated in battle.
//
// To test this go here: https://www.shadertoy.com/view/NsBBzd.

// Produces a hash of the vec2 in the interval [0, 1). This seems
// to produce best results when the input vector is in the in-
// terval ~ [0, 1] approximately.
float hash_vec2( in vec2 vec ) {
  // Taken from https://stackoverflow.com/a/4275343.
  vec2 magic_vec2 = vec2( 12.9898, 78.233 );
  float magic_float = 43758.5453;
  return fract( sin( dot( vec, magic_vec2 ) )*magic_float );
}

vec4 depixelate( in vec4 color ) {
  // FIXME: change 1000.0 to largest screen size dimension.
  // We need to divide by 1000.0 to put the input in a good range
  // for the hash function, otherwise we get repeating patterns.
  float hash = hash_vec2( position/1000.0 );
  return vec4( color.rgb, hash > frag_depixelate );
}

/****************************************************************
** Alpha scaling.
*****************************************************************/
vec4 alpha( in vec4 color ) {
  return vec4( color.rgb, color.a*in_alpha_multiplier );
}

/****************************************************************
** main
*****************************************************************/
void main() {
  // Default color red to catch errors.
  vec4 color = vec4( 1, 0, 0, 1 );

  // Basic type stage.
  switch( frag_type ) {
    case 0: color = type_sprite();     break;
    case 1: color = type_solid();      break;
    case 2: color = type_silhouette(); break;
  }

  // Post processing.

  if( frag_depixelate > 0.0 ) color = depixelate( color );
  if( frag_alpha_multiplier < 1.0 ) color = alpha( color );

  final_color = color;
}
