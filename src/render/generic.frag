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

flat in int   frag_type;
flat in vec3  frag_depixelate;
     in vec2  frag_position;
     in vec2  frag_atlas_position;
flat in vec2  frag_atlas_target_offset;
     in vec4  frag_fixed_color;
     in float frag_alpha_multiplier;
flat in float frag_scaling;
flat in int   frag_color_cycle;

uniform sampler2D u_atlas;
uniform vec2 u_atlas_size;
// Screen dimensions in the game's logical pixel units.
uniform vec2 u_screen_size;
uniform int u_color_cycle_stage;

out vec4 final_color;

/****************************************************************
** Helpers
*****************************************************************/
vec4 atlas_lookup( in vec2 atlas_pixel_pos ) {
  return texture( u_atlas, atlas_pixel_pos/u_atlas_size );
}

/****************************************************************
** Sprites.
*****************************************************************/
vec4 type_sprite() {
  return atlas_lookup( frag_atlas_position );
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
** Stencils.
*****************************************************************/
vec4 type_stencil() {
  vec4 candidate = atlas_lookup( frag_atlas_position );
  if( candidate.rgb != frag_fixed_color.rgb )
    return candidate;
  // We have the key color, so replace it with a pixel from the
  // alternate sprite.
  vec4 target_color = atlas_lookup( frag_atlas_position +
                                    frag_atlas_target_offset );
  return vec4( target_color.rgb, target_color.a*candidate.a );
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

float hash_position() {
  // We need to divide by this screen scale to put the input in a
  // good range (approximately in the range [0,1]) for the hash
  // function to yield good results, otherwise we get repeating
  // patterns.
  float screen_scale = u_screen_size.x;
  // The position that we will hash will be 1) a position that is
  // relative to an anchor position so that the sprite will de-
  // pixelate deterministically even if it is moving on screen
  // while doing so, and 2) unscaled so that just in case the
  // sprite is scaled we will still depixelate the sprite's
  // pixels (which may be larger or smaller than the logical
  // pixels if we are zoomed).
  vec2 anchor = frag_depixelate.xy;
  vec2 hash_position = (frag_position-anchor)/frag_scaling;
  return hash_vec2( floor( hash_position )/screen_scale );
}

vec4 depixelate_to( in vec4 c1, in vec4 c2 ) {
  float animation_stage = frag_depixelate.z;
  return ( hash_position() > animation_stage ) ? c1 : c2;
}

/****************************************************************
** Alpha scaling.
*****************************************************************/
vec4 alpha( in vec4 color ) {
  return vec4( color.rgb, color.a*frag_alpha_multiplier );
}

/****************************************************************
** Color Cycling
*****************************************************************/
// When color cycling is enabled for a triangle it means that
// this shader will, after computing the final color, look it up
// in the source array. If not present then the color will be
// emitted as is. If it is present then the color will be re-
// placed with a color in the destination array, at an index de-
// termined by the current color cycling stage. Should probably
// replace these with uniforms.
const vec3 color_cycle_src[5] = vec3[](
  vec3( 50 ), vec3( 100 ), vec3( 150 ), vec3( 200 ), vec3( 250 )
);

const vec3 S = vec3( 90, 122, 148 ); // surf color.
const vec4 X = vec4( 0 );            // clear.

const vec4 color_cycle_dst[10] = vec4[](
  vec4( S, 230 ), vec4( S, 115 ), vec4( S, 50 ),
  X, X, X, X, X, X, X
);

vec4 color_cycle( in vec4 color ) {
  // We have a color in the range [0,1] but the src and dst
  // colors are specified as [0,255]. So to make the comparison
  // between the two immune to rounding errors we will convert
  // the [0,1] color to [0,255] with rounding.
  vec3 rgb_ubyte = round( color.rgb*255.0 );
  for( int i = 0; i < color_cycle_src.length(); ++i ) {
    if( color_cycle_src[i] == rgb_ubyte ) {
      int dst_idx = (i + u_color_cycle_stage)
                  % color_cycle_dst.length();
      vec4 dst = color_cycle_dst[dst_idx]/255.0;
      // Overwrite the color but with alpha mixing.
      return vec4( dst.rgb, dst.a*color.a );
    }
  }
  return color;
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
    case 3: color = type_stencil();    break;
  }

  // Depixelation.
  if( frag_depixelate.z > 0.0 ) {
    // Depixelate to nothing by default.
    vec4 target_color = vec4( 0.0 );
    // Check if we are depixelating to another sprite. This re-
    // quires that we have the offset to the other sprite and
    // also requires that this is a texture to begin with so that
    // it won't affect the nationality flag. This logic may need
    // to be improved at some point.
    if( frag_type == 0 && length( frag_atlas_target_offset ) > 0 ) {
      // Depixelate to another sprite, so get the position and
      // color of the pixel in the other texture that we're de-
      // pixelating to.
      target_color = atlas_lookup( frag_atlas_position +
                                   frag_atlas_target_offset );
    }
    color = depixelate_to( color, target_color );
  }

  // Color cycling.
  if( frag_color_cycle != 0 ) color = color_cycle( color );

  // Alpha.
  if( frag_alpha_multiplier < 1.0 ) color = alpha( color );

  final_color = color;
}
