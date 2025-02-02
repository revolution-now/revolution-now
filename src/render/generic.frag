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
flat in int   frag_color_cycle_plan;
flat in int   frag_downsample;
flat in int   frag_color_cycle;
flat in int   frag_desaturate;
flat in int   frag_use_fixed_color;
flat in int   frag_uniform_depixelation;
flat in float frag_depixelate_stage;
flat in float frag_depixelate_inverted;
flat in vec2  frag_depixelate_anchor;
flat in vec4  frag_depixelate_stages;
flat in vec4  frag_depixelate_stages_unscaled;
     in vec2  frag_position;
     in vec2  frag_atlas_position;
flat in vec4  frag_atlas_rect;
flat in vec2  frag_atlas_target_offset;
     in vec4  frag_stencil_key_color;
     in vec4  frag_fixed_color;
     in float frag_alpha_multiplier;
flat in float frag_scaling;
flat in vec2  frag_default_hash_anchor;

uniform sampler2D u_atlas;
uniform vec2 u_atlas_size;
// Screen dimensions in the game's logical pixel units.
uniform vec2 u_screen_size;

// Stage of the global depixelation. This can be used to achieve
// two different things:
//   1. it can be used to allow depixelation of something without
//      regenerate the vertices each frame, and
//   2. it can be used to allow depixelating things that are al-
//      ready normally in a stage of depixelation.
uniform float u_depixelation_stage;

// Color cycling.
const int CYCLE_PLAN_SPAN = 10; // Must match config/gfx.
const int NUM_CYCLE_PLANS = 3;  // Must match config/gfx.
const int CYCLE_ARR_SIZE = NUM_CYCLE_PLANS*CYCLE_PLAN_SPAN;
uniform int u_color_cycle_stage;
uniform ivec4 u_color_cycle_targets[CYCLE_ARR_SIZE];
uniform ivec3 u_color_cycle_keys[CYCLE_PLAN_SPAN];

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
  const float BUFFER = 0.002;
  vec2 delta = vec2( BUFFER )/u_atlas_size;
  vec2 atlas_pos_min = bounds.xy+delta;
  vec2 atlas_pos_max = bounds.xy+bounds.zw-delta;
  return clamp( atlas_pos, atlas_pos_min, atlas_pos_max );
}

// Downsample the sprite by the given factor. E.g., if factor is
// 2 then the sprite will be rendered at the same size, but will
// have only half as many "pixels" rendered along each dimension,
// so pixels will appear twice as big. It does this by throwing
// away every second pixel.
vec2 downsample_sprite( in vec2 atlas_pos, in vec4 atlas_rect,
                        in float factor ) {
  // We need to round down, but we need to do so with the origin
  // at the sprite's origin in the atlas, otherwise our results
  // are at the mercy of the (arbitrary) location that the sprite
  // happens to be in the atlas.
  vec2 relative = atlas_pos-atlas_rect.xy;
  // This does the downsampling; it effectively rounds down to
  // the nearest multiple of factor.
  relative = floor( relative/factor )*factor;
  // After the above transformation our atlas coordinate is going
  // to be an integer, which means that it is referring to the
  // point/line right between two pixels in the atlas. In these
  // circumstances there is sometimes an ambiguity to the sampler
  // as to which pixel to actually take, which distorts things.
  // The fix_atlas_pos function fixes this for pixels at the
  // edges of the sprite, but this is needed for the pixels in
  // the middle, and will put the coordinate such that it is
  // squarely within the area of the pixel we want so that there
  // is no ambiguity to the sampler. That's the theory anyway...
  relative = relative + vec2( .01 );
  return atlas_rect.xy + relative;
}

vec4 atlas_lookup( in vec2 atlas_pos, in vec4 atlas_rect ) {
  if( frag_downsample != 0 ) {
    int n = 1;
    for( int i = 0; i < frag_downsample; ++i ) n *= 2;
    atlas_pos = downsample_sprite( atlas_pos, atlas_rect, n );
  }
  atlas_pos /= u_atlas_size;
  atlas_rect /= vec4(u_atlas_size.xy, u_atlas_size.xy);
  atlas_pos = fix_atlas_pos( atlas_pos, atlas_rect );
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
** Stencils.
*****************************************************************/
vec4 type_stencil() {
  vec4 candidate = atlas_lookup( frag_atlas_position,
                                 frag_atlas_rect );
  if( candidate.rgb != frag_stencil_key_color.rgb )
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

float hash_position( in vec2 anchor ) {
  // The position that we will hash will be 1) a position that is
  // relative to an anchor position so that the sprite will de-
  // pixelate deterministically even if it is moving on screen
  // while doing so, and 2) unscaled so that just in case the
  // sprite is scaled we will still depixelate the sprite's
  // pixels (which may be larger or smaller than the logical
  // pixels if we are zoomed).
  vec2 hashable = (frag_position - anchor)/frag_scaling;
  // This basically determines the max possible "wave-length" of
  // the repeating hash pattern in logical pixels. The number 320
  // is chosen because it is on the order of the width of the
  // land view in logical pixels (i.e., about ten tiles across *
  // 32 pixel width). This doesn't have to be exact, it just has
  // to serve as a divisor that will map the logical pixels
  // across the screen to [0,1) so that the hash function will
  // produce good (and non-repeating) results across the screen.
  // The reason that we don't use u_screen_size.x here (which
  // would give us the real screen size in logical pixels) is
  // that then as we resize the main game window and/or scale up-
  // /down the logical screen resolution, the depixelation pat-
  // tern changes, which looks kind of odd.
  float fixed_approx_screen_width_logical = 320.0;
  // Use floor() so that all physical pixels inside a logical
  // pixel are treated the same way, in order to create the illu-
  // sion of low-resolution pixelated graphics. Use fract() to
  // put the hash input in the range [0,1) for hash function to
  // yield good results, otherwise we could get repeating pat-
  // terns.
  return hash_vec2( fract( floor( hashable ) /
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
  float stage_base = frag_depixelate_stage;
  float stage = stage_base + stage_deltas.x + stage_deltas.y;
  return stage;
}

float depixelate() {
  vec2 anchor = frag_depixelate_anchor;
  bool on = ( hash_position( anchor ) > depixel_stage() );
  if( frag_depixelate_inverted != 0.0 ) on = !on;
  return on ? 1.0 : 0.0;
}

float uniform_depixelate() {
  // The anchor used here must not be related to the anchor used
  // by the normal depixelation process. This is because we may
  // be applying this uniform depixelation to a composite ren-
  // dering consisting of multiple things rendered on top of each
  // other, each with different origins and different depixela-
  // tion anchors that produced them. If each of those components
  // has a different anchor, and if we use those anchors for this
  // uniform depixelation, then that means that a given pixel
  // could be on in one component but off in another component,
  // producing visual artifacts. Therefore, we have to use an an-
  // chor that is fixed for all components that are under the
  // uniform depixelation. That said, the anchor still needs to
  // be scaled and translated appropriately so that things remain
  // consistent as the screen is scrolled. As a result, note that
  // this uniform depixelation won't really work with an indi-
  // vidual sprite that is moving on screen via a mechanism other
  // than the usual scale/translate mods. But that is probably OK
  // because such a graphic would likely need to be re-rendered
  // each frame, in which case we could just use normal depixela-
  // tion for it. This "uniform" depixelation is really just for
  // rendering and pixelating complex buffers that we don't want
  // to re-render each frame and/or we can't depixelate them
  // using the normal depixelation mod because they are already
  // normally composed of some kind of depixelation (thus we need
  // a separate depixelation mechanism).
  vec2 anchor = frag_default_hash_anchor;
  bool on = ( hash_position( anchor ) > u_depixelation_stage );
  return on ? 1.0 : 0.0;
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
// termined by the current color cycling stage.
vec4 color_cycle( in vec4 color ) {
  // We have a color in the range [0,1] but the src and dst
  // colors are specified as [0,255]. So to make the comparison
  // between the two immune to rounding errors we will convert
  // the [0,1] color to [0,255] with rounding.
  vec3 rgb_ubyte = round( color.rgb*255.0 );
  int plan_start = frag_color_cycle_plan*CYCLE_PLAN_SPAN;
  for( int i = 0; i < u_color_cycle_keys.length(); ++i ) {
    if( u_color_cycle_keys[i] != rgb_ubyte ) continue;
    int dst_idx = (i + u_color_cycle_stage) % CYCLE_PLAN_SPAN;
    vec4 dst = u_color_cycle_targets[plan_start+dst_idx]/255.0;
    // This next line serves no purpose but seems to be needed
    // to work around a strange issue (driver bug?) on Mac OS
    // causing strange visual artifacts to appear.
    (dst.a != 0 ? dst.a : dst.a);
    // Overwrite the color but with alpha mixing.
    return vec4( dst.rgb, dst.a*color.a );
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
// To fully desaturate we could first conver to HSL, then zero
// the saturation, then convert back to RGB, but it seems that
// the following simpler approach produces the same result.
//
// The human eye tends to be more sensitive to green colors and
// the least to blue. So to get the most physically accurate re-
// sults we'll need to use weighted channels. This is as opposed
// to just taking the average of the three (i.e., multiplying
// each by .3333). These numbers are taken from learnopengl.com.
vec3 desaturate_fast( in vec3 c ) {
  return vec3( 0.2126*c.r + 0.7152*c.g + 0.0722*c.b );
}

// This is not accurate for large factors, e.g. 1.5. But for
// small factors it should be ok.
vec3 saturate_fast( in vec3 c, float factor ) {
  float avg = (c.r+c.g+c.b)/3;
  vec3 delta = c-avg;
  delta *= factor;
  return delta+avg;
}

vec3 saturate_slow( in vec3 c, in float factor ) {
  c.rgb = rgb_to_hsl( c.rgb );
  c.g *= factor;
  if( c.g > 1.0 ) c.g = 1.0;
  c.rgb = hsl_to_rgb( c.rgb );
  return c;
}

/****************************************************************
** Testing/Debugging
*****************************************************************/
vec4 use_me() {
  vec4 dummy = vec4( 0 )
    // List variables here to "use".
    + vec4( frag_color_cycle_plan )
    ;
  return .000000000000000000000001*dummy;
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
    case 2: color = type_stencil();    break;
  }

  // Depixelation.
  bool depixel_enabled = frag_depixelate_stage != 0 ||
                         frag_depixelate_stages.zw != vec2( 0, 0 );
  if( depixel_enabled ) color *= depixelate();

  // Color cycling.
  if( frag_color_cycle != 0 ) color = color_cycle( color );

  // Alpha.
  if( frag_alpha_multiplier < 1.0 ) color = alpha( color );

  // Color fixing.
  if( frag_use_fixed_color != 0 ) color.rgb = frag_fixed_color.rgb;

  // Desaturation.
  if( frag_desaturate != 0 ) color.rgb = desaturate_fast( color.rgb );

  // Uniform depixelation.
  if( frag_uniform_depixelation != 0 ) color *= uniform_depixelate();

  // color.rgb = saturate_slow( color.rgb, 1.5 );

  final_color = color + use_me();
}
