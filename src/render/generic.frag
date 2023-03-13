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
flat in int   frag_color_cycle;
flat in int   frag_desaturate;
flat in vec4  frag_depixelate;
flat in vec4  frag_depixelate_stages;
flat in vec4  frag_depixelate_stages_unscaled;
     in vec2  frag_position;
     in vec2  frag_atlas_position;
flat in vec4  frag_atlas_rect;
flat in vec2  frag_atlas_target_offset;
     in vec4  frag_fixed_color;
     in float frag_alpha_multiplier;
flat in float frag_scaling;

uniform sampler2D u_atlas;
uniform vec2 u_atlas_size;
// Screen dimensions in the game's logical pixel units.
uniform vec2 u_screen_size;
uniform int u_color_cycle_stage;

out vec4 final_color;

/****************************************************************
** Helpers
*****************************************************************/
// Fix atlas coordinate sampling edge bleeing. It happens some-
// times that when a scaling or zoom has been applied, pixels
// that are right on the boundary of sprites can (apparently due
// to interpolation and/or rounding) end up sampling the atlas
// just outside of the target sprite, which leads to unsightly
// artifacts when rendered (horizontal and vertical lines appear
// at the edges of some sprites at some zoom levels). To fix
// this, we will use the atlas_rect, a flat per-sprite vertex at-
// tribute, which gives the bounding rect of a sprite in the at-
// las. We will clamp the atlas coordinate to within this box.
// Actually, this clamping is apparently not enough; we need to
// clamp it to within a slightly smaller box by removing a tiny
// buffer around the edge. This finally seems to guarantee that
// we don't sample outside of the sprite. that if some day these
// artifacts appear again and this needs to be made more aggre-
// sive then the buf number can be slightly increased (it has
// units of atlas pixels). See for example:
//
//   stackoverflow.com/questions/33307779/
//       opengl-4-4-texture-atlas-artifacts
//
vec2 fix_atlas_pos( in vec2 atlas_pos, in vec4 bounds ) {
  float BUFFER = 0.002;
  vec2 delta = vec2( BUFFER )/u_atlas_size;
  vec2 atlas_pos_min = vec2(bounds.x+delta.x, bounds.y+delta.y);
  vec2 atlas_pos_max = vec2(bounds.x+bounds.z-delta.x,
                            bounds.y+bounds.w-delta.y);
  return clamp( atlas_pos, atlas_pos_min, atlas_pos_max );
}

vec4 atlas_lookup( in vec2 atlas_pos, in vec4 atlas_rect ) {
  atlas_pos /= u_atlas_size;
  atlas_rect /= vec4(u_atlas_size.xy, u_atlas_size.xy);
  atlas_pos =fix_atlas_pos( atlas_pos, atlas_rect );
  return texture( u_atlas, atlas_pos );
}

/****************************************************************
** Sprites.
*****************************************************************/
vec4 type_sprite() {
  return atlas_lookup( frag_atlas_position, frag_atlas_rect );
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
  vec4 candidate = atlas_lookup( frag_atlas_position,
                                 frag_atlas_rect );
  if( candidate.rgb != frag_fixed_color.rgb )
    return candidate;
  // We have the key color, so replace it with a pixel from the
  // alternate sprite.
  vec4 target_atlas_rect = frag_atlas_rect;
  target_atlas_rect.xy += frag_atlas_target_offset;
  vec4 target_color = atlas_lookup( frag_atlas_position +
                                    frag_atlas_target_offset,
                                    target_atlas_rect );
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
  // The position that we will hash will be 1) a position that is
  // relative to an anchor position so that the sprite will de-
  // pixelate deterministically even if it is moving on screen
  // while doing so, and 2) unscaled so that just in case the
  // sprite is scaled we will still depixelate the sprite's
  // pixels (which may be larger or smaller than the logical
  // pixels if we are zoomed).
  vec2 anchor = frag_depixelate.xy;
  vec2 hash_position = (frag_position-anchor)/frag_scaling;
  // The number 320 is chosen because it is on the order of the
  // width of the land view in logical pixels (i.e., about ten
  // tiles across * 32 pixel width). This doesn't have to be ex-
  // act, it just has to serve as a divisor that will map the
  // logical pixels across the screen to [0,1) so that the hash
  // function will produce good (and non-repeating) results
  // across the screen. The reason that we don't use
  // u_screen_size.x here (which would give us the real screen
  // size in logical pixels) is that then as we resize the main
  // game window and/or scale up/down the logical screen resolu-
  // tion, the depixelation pattern changes, which looks kind of
  // odd (although it is harmless).
  float fixed_approx_screen_width_logical = 320.0;
  // Use floor() so that all physical pixels inside a logical
  // pixel are treated the same way, in order to create the illu-
  // sion of low-resolution pixelated graphics. Use fract() to
  // put the hash input in the range [0,1) for hash function to
  // yield good results, otherwise we could get repeating pat-
  // terns.
  return hash_vec2( fract( floor( hash_position ) /
                           fixed_approx_screen_width_logical ) );
}

// This function will compute the depixelation stage by taking
// the base stage and extrapolating it over the triangle using
// the gradient. The reason that we don't let OpenGL interpolate
// this for us is because 1) then we'd have to set the depixela-
// tion stage differently at each vertex in the triangle which is
// more complex for the renderer, and 2) we wouldn't have a way
// to sample the depixelation stage at the upper left corner of
// the logic pixel which is what we need to maintain the illusion
// of logical pixels and is what we do below (otherwise the stage
// would vary continuously within the logical pixels and produce
// differently-sized pixels). #1 is an inconvenience, but #2
// proved to be a deal breaker; hence we do it the way we do it.
float depixel_stage() {
  // When depixelation is gradiated, the anchor is required to
  // represent the upper left of the tile (a stronger requirement
  // than it just being fixed relative to the points on the tri-
  // angle, which is the requirement for non-gradiated depixela-
  // tion).
  vec2 stage_anchor = frag_depixelate_stages.xy;
  // We need to floor the distance from the anchor so that we
  // sample the depixelation stage at a fixed point within each
  // logical pixel. If we don't do this them the depixelation
  // starts to appear within logical pixels, which does not look
  // right since it yields pixels of different sizes.
  vec2 dist_from_anchor =
      floor( (frag_position-stage_anchor)/frag_scaling );
  // The gradient has units of inverse scaled logical pixels, so
  // we need to unscale it because we're multiplying by the dis-
  // tance from the anchor which was unscaled (for its own rea-
  // sons, see above). To do that, theoretically, we multiply by
  // the frag_scaling (we can't cancel out this factor with the
  // one in the dist_from_anchor because that one needs to be
  // floor'd first). That works most of the time, but occasion-
  // ally produces some weird visual flickering when zooming in
  // probably due to rounding errors caused by first scaling the
  // gradient (in the vertex shader) then unscaling the same num-
  // ber. So we just use the unscaled version directly.
  vec2 stage_deltas = frag_depixelate_stages_unscaled.zw*
                      dist_from_anchor;
  // Think of the stage as a 2d plane, f(x,y). We know the value
  // at the upper left corner of the tile (stage_base) and we
  // need to extrapolate down to our point (really, the upper
  // left corner of the logical pixel that we're currently in).
  float stage_base = frag_depixelate.z;
  float stage = stage_base + stage_deltas.x + stage_deltas.y;
  return stage;
}

vec4 depixelate( in vec4 c ) {
  float animation_stage = depixel_stage();
  bool on = ( hash_position() > animation_stage );
  float inverted = frag_depixelate.w;
  if( inverted != 0.0 ) on = !on;
  return on ? c : vec4( 0.0 );
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

// const vec3 S = vec3( 90, 122, 148 ); // surf color.
const vec3 S = vec3( 97, 128, 153 ); // surf color.
const vec4 X = vec4( 0 );            // clear.

const vec4 color_cycle_dst[9] = vec4[](
  vec4( S, 230 ), vec4( S, 115 ), vec4( S, 50 ),
  X, X, X, X, X, X
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
      // This next line serves no purpose but seems to be needed
      // to work around a strange issue (driver bug?) on Mac OS
      // causing strange visual artifacts to appear.
      (dst.a != 0 ? dst.a : dst.a);
      // Overwrite the color but with alpha mixing.
      return vec4( dst.rgb, dst.a*color.a );
    }
  }
  return color;
}

/****************************************************************
** RGB <--> HSL conversions.
*****************************************************************/
// See http://www.chilliant.com/rgb2hsv.html.
const float EPSILON = 1e-7;

// Hue [0..1] to RGB [0..1].
vec3 hue_to_rgb( in float hue ) {
  vec3 rgb = abs( hue*6 - vec3( 3, 2, 4 ) )*vec3( 1, -1, -1 )
           + vec3( -1, 2, 2 );
  return clamp( rgb, 0, 1 );
}

// RGB [0..1] to Hue-Chroma-Value [0..1].
vec3 rgb_to_hcv( in vec3 rgb ) {
  vec4 p = (rgb.g < rgb.b) ? vec4( rgb.bg, -1,  2./3. )
                           : vec4( rgb.gb,  0, -1./3. );
  vec4 q = (rgb.r < p.x) ? vec4( p.xyw, rgb.r )
                         : vec4( rgb.r, p.yzx );
  float c = q.x - min( q.w, q.y );
  float h = abs( (q.w - q.y)/(6*c + EPSILON) + q.z );
  return vec3( h, c, q.x );
}

// Hue-Saturation-Lightness [0..1] to RGB [0..1].
vec3 hsl_to_rgb( in vec3 hsl ) {
  vec3 rgb = hue_to_rgb( hsl.x );
  float c = (1 - abs( 2*hsl.z - 1 ))*hsl.y;
  return (rgb - .5)*c + hsl.z;
}

// RGB [0..1] to Hue-Saturation-Lightness [0..1].
vec3 rgb_to_hsl( in vec3 rgb ) {
  vec3 hcv = rgb_to_hcv( rgb );
  float z = hcv.z - hcv.y*.5;
  float s = hcv.y/(1 - abs( z*2 - 1 ) + EPSILON);
  return vec3( hcv.x, s, z );
}

/****************************************************************
** De-saturation.
*****************************************************************/
vec3 desaturate( in vec3 color ) {
  color = rgb_to_hsl( color.rgb );
  color.y = 0; // zero saturation.
  color = hsl_to_rgb( color.xyz );
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
  bool depixel_enabled = frag_depixelate.z != 0 ||
                         frag_depixelate_stages.zw != vec2( 0, 0 );
  if( depixel_enabled )
    color = depixelate( color );

  // Color cycling.
  if( frag_color_cycle != 0 ) color = color_cycle( color );

  // Alpha.
  if( frag_alpha_multiplier < 1.0 ) color = alpha( color );

  // Desaturation.
  if( frag_desaturate != 0 ) color.rgb = desaturate( color.rgb );

  final_color = color;
}
