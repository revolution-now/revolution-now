/****************************************************************
**sdl-util.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Interface for calling SDL functions
*
*****************************************************************/
#include "sdl-util.hpp"

// Revolution Now
#include "config-files.hpp"
#include "errors.hpp"
#include "fmt-helper.hpp"
#include "frame.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "screen.hpp"

// base-util
#include "base-util/algo.hpp"

// SDL
#include "SDL_mixer.h"

// c++ standard library
#include <cmath>
#include <iomanip>
#include <unordered_map>
#include <vector>

using namespace std;

namespace rn {

namespace {

auto g_pixel_format = ::SDL_PIXELFORMAT_RGBA8888;

// Must be unordered_map since we need pointer stability; other
// modules will hold references to these.
unordered_map<string, Texture> loaded_textures;

vector<Rect> clip_stack;

int g_current_render_target{-1};
int g_next_texture_id{1};

} // namespace

::SDL_Rect to_SDL( Rect const& rect ) {
  ::SDL_Rect res;
  res.x = rect.x._;
  res.y = rect.y._;
  res.w = rect.w._;
  res.h = rect.h._;
  return res;
}

ND ::SDL_Point to_SDL( Coord const& coord ) {
  ::SDL_Point p;
  p.x = coord.x._;
  p.y = coord.y._;
  return p;
}

ND Rect from_SDL( ::SDL_Rect const& rect ) {
  Rect res;
  res.x = rect.x;
  res.y = rect.y;
  res.w = rect.w;
  res.h = rect.h;
  return res;
}

void init_sdl() {
  CHECK( ::SDL_Init( SDL_INIT_EVERYTHING ) >= 0,
         "sdl could not initialize" );

  // Open Audio device
  CHECK( !Mix_OpenAudio( config_sound.general.frequency,
                         AUDIO_S16SYS,
                         config_sound.general.channels,
                         config_sound.general.chunk_size ),
         "could not open audio: Mix_OpenAudio ERROR: {}",
         ::Mix_GetError() );

  // Set Volume
  constexpr int default_volume{10};

  ::Mix_VolumeMusic( default_volume );
}

// All the functions in this method should not cause problems
// even if their corresponding initialization routines were
// not successfully run.
void cleanup_sdl() {
  for( auto& p : loaded_textures ) p.second.free();
  ::SDL_Quit();
}

REGISTER_INIT_ROUTINE( sdl, init_sdl, cleanup_sdl );

Texture from_SDL( ::SDL_Texture* tx ) { return Texture( tx ); }

::SDL_Surface* optimize_surface( ::SDL_Surface* in,
                                 bool           release_input ) {
  auto* fmt = ::SDL_AllocFormat( g_pixel_format );
  CHECK( fmt != nullptr );
  auto* optimized = ::SDL_ConvertSurface( in, fmt, 0 );
  CHECK( optimized != nullptr );
  ::SDL_FreeFormat( fmt );
  if( release_input ) ::SDL_FreeSurface( in );
  return optimized;
}

::SDL_Surface* load_surface( const char* file ) {
  SDL_Surface* surface = IMG_Load( file );
  CHECK( surface, "failed to load image: {}", file );
  return surface;
}

Texture& load_texture( const char* file ) {
  SDL_Surface* image = IMG_Load( file );
  CHECK( image != nullptr, "failed to load image {}", file );
  loaded_textures[string( file )] =
      Texture::from_surface( image );
  return loaded_textures[string( file )];
}

Texture& load_texture( fs::path const& path ) {
  return load_texture( path.string().c_str() );
}

Delta texture_delta( Texture const& texture ) {
  if( !texture ) return main_window_logical_size();
  int w, h;
  CHECK(
      !::SDL_QueryTexture( texture, nullptr, nullptr, &w, &h ) );
  return {W{w}, H{h}};
}

void set_render_target( Texture const& tx ) {
  if( g_current_render_target == tx.id() ) return;
  CHECK( !::SDL_SetRenderTarget( g_renderer, tx.get() ) );
  g_current_render_target = tx.id();
  event_counts()["set-render-target"].tick();
}

void push_clip_rect( Rect const& rect ) {
  ::SDL_Rect sdl_rect;
  ::SDL_RenderGetClipRect( g_renderer, &sdl_rect );
  clip_stack.emplace_back( from_SDL( sdl_rect ) );
  sdl_rect = to_SDL( rect );
  ::SDL_RenderSetClipRect( g_renderer, &sdl_rect );
}

void pop_clip_rect() {
  CHECK( !clip_stack.empty() );
  auto rect = clip_stack.back();
  clip_stack.pop_back();
  ::SDL_Rect sdl_rect = to_SDL( rect );
  ::SDL_RenderSetClipRect( g_renderer, &sdl_rect );
}

void copy_texture( Texture const& from, Texture const& to,
                   Rect const& src, Rect const& dst,
                   double angle, SDL_RendererFlip flip ) {
  set_render_target( to );
  auto sdl_src = to_SDL( src );
  auto sdl_dst = to_SDL( dst );
  CHECK( !::SDL_RenderCopyEx( g_renderer, from, &sdl_src,
                              &sdl_dst, angle, nullptr, flip ) );
}

// With alpha. TODO: figure out why this doesn't behave like a
// standard copy_texture when alpha == 255.
void copy_texture_alpha( Texture const& from, Texture const& to,
                         Coord const& dst_coord,
                         uint8_t      alpha ) {
  ::SDL_Texture* target = to.get();
  CHECK( from );
  ::SDL_SetTextureBlendMode( from, ::SDL_BLENDMODE_BLEND );
  ::SDL_SetTextureBlendMode( target, ::SDL_BLENDMODE_BLEND );
  CHECK( !::SDL_SetTextureAlphaMod( from.get(), alpha ) );
  set_render_target( to );
  auto rect     = Rect::from( dst_coord, texture_delta( from ) );
  auto sdl_rect = to_SDL( rect );
  CHECK( !::SDL_RenderCopy( g_renderer, from, nullptr,
                            &sdl_rect ) );
  // Restore texture's alpha because that is what most actions
  // will need it to be, and we don't set it before every texture
  // copying action in this module.
  ::SDL_SetTextureAlphaMod( from.get(), 255 );
}

void copy_texture( Texture const& from, Texture const& to,
                   Coord const& dst_coord ) {
  ::SDL_Texture* target = to.get();
  CHECK( from );
  ::SDL_SetTextureBlendMode( from, ::SDL_BLENDMODE_BLEND );
  ::SDL_SetTextureBlendMode( target, ::SDL_BLENDMODE_BLEND );
  set_render_target( to );
  auto rect     = Rect::from( dst_coord, texture_delta( from ) );
  auto sdl_rect = to_SDL( rect );
  CHECK( !::SDL_RenderCopy( g_renderer, from, nullptr,
                            &sdl_rect ) );
}

void copy_texture_to_main( Texture const& from ) {
  copy_texture( from, Texture{}, Coord{} );
}

void copy_texture( Texture const& from, Texture const& to ) {
  copy_texture( from, to, Coord{} );
}

void copy_texture_stretch( Texture const& from,
                           Texture const& to, Rect const& src,
                           Rect const& dest ) {
  ::SDL_Texture* target = to.get();
  ::SDL_SetTextureBlendMode( from, ::SDL_BLENDMODE_BLEND );
  ::SDL_SetTextureBlendMode( target, ::SDL_BLENDMODE_BLEND );
  set_render_target( to );
  ::SDL_Rect sdl_src  = to_SDL( src );
  ::SDL_Rect sdl_dest = to_SDL( dest );
  CHECK( !::SDL_RenderCopy( g_renderer, from, &sdl_src,
                            &sdl_dest ) );
}

Texture clone_texture( Texture const& tx ) {
  auto res = create_texture_transparent( tx.size() );
  copy_texture( tx, res );
  return res;
}

Texture create_texture( W w, H h ) {
  auto tx = from_SDL( ::SDL_CreateTexture(
      g_renderer, g_pixel_format, SDL_TEXTUREACCESS_TARGET, w._,
      h._ ) );
  clear_texture_black( tx );
  return tx;
}

ND Texture create_texture( Delta delta ) {
  return create_texture( delta.w, delta.h );
}

ND Texture create_texture( Delta delta, Color const& color ) {
  auto tx = create_texture( delta.w, delta.h );
  fill_texture( tx, color );
  return tx;
}

ND Texture create_texture_transparent( Delta delta ) {
  auto tx = create_texture( delta.w, delta.h );
  clear_texture_transparent( tx );
  return tx;
}

ND Texture create_screen_physical_sized_texture() {
  auto res = create_texture( screen_physical_size() );
  logger->debug( "created screen-sized texture occupying {}MB.",
                 res.mem_usage_mb() );
  return res;
}

::SDL_Surface* create_surface( Delta delta ) {
  auto* fmt = ::SDL_AllocFormat( g_pixel_format );

  SDL_Surface* surface = SDL_CreateRGBSurface(
      0, delta.w._, delta.h._, fmt->BitsPerPixel, 0, 0, 0, 0 );

  ::SDL_FreeFormat( fmt );
  CHECK( surface != nullptr, "SDL_CreateRGBSurface failed" );
  return optimize_surface( surface, /*release_input=*/true );
  ;
}

Matrix<Color> texture_pixels( Texture const& tx ) {
  auto  delta   = texture_delta( tx );
  auto* surface = create_surface( delta );

  auto* fmt = ::SDL_AllocFormat( g_pixel_format );

  set_render_target( tx );
  ::SDL_RenderReadPixels( g_renderer, NULL, g_pixel_format,
                          surface->pixels, surface->pitch );

  // This is the only one we use in the game for rendering.
  CHECK( fmt->BitsPerPixel == 32 );

  logger->debug( "reading texture pixel data of size {}",
                 delta );

  Matrix<Color> res( delta );
  SDL_LockSurface( surface );

  auto rect = Rect::from( Coord{}, delta );
  for( auto coord : rect ) {
    ASSIGN_CHECK_OPT( idx, rect.rasterize( coord ) );
    Uint32 pixel = ( (Uint32*)surface->pixels )[idx];
    res[coord]   = from_SDL( color_from_pixel( fmt, pixel ) );
  }

  ::SDL_UnlockSurface( surface );
  ::SDL_FreeFormat( fmt );
  ::SDL_FreeSurface( surface );

  return res;
}

Texture create_shadow_texture( Texture const& tx ) {
  auto cloned = clone_texture( tx );
  auto white =
      create_texture( tx.size(), Color{255, 255, 255, 255} );
  // black.a should not be relevant here.
  auto black = create_texture( tx.size(), Color{0, 0, 0, 255} );

  // The process will be done in two stages; note that each stage
  // respects alpha gradations when performing its action, so
  // that the resulting texture will maintain the same alpha
  // pattern as the input texture.
  set_render_target( cloned );

  // Stage one: turn the cloned texture all white in its opaque
  // parts.
  //   dstRGB = (srcRGB * srcA) + dstRGB
  //   dstA = dstA
  ::SDL_SetTextureBlendMode( cloned, ::SDL_BLENDMODE_ADD );
  ::SDL_SetTextureBlendMode( white, ::SDL_BLENDMODE_ADD );
  CHECK(
      !::SDL_RenderCopy( g_renderer, white, nullptr, nullptr ) );

  // Stage two: turn the white parts of the cloned texture to
  // black.
  //   dstRGB = srcRGB * dstRGB
  //   dstA = dstA
  ::SDL_SetTextureBlendMode( cloned, ::SDL_BLENDMODE_MOD );
  ::SDL_SetTextureBlendMode( black, ::SDL_BLENDMODE_MOD );
  CHECK(
      !::SDL_RenderCopy( g_renderer, black, nullptr, nullptr ) );

  // Return blend mode to a standard value just for good measure.
  ::SDL_SetTextureBlendMode( cloned, ::SDL_BLENDMODE_BLEND );

  return cloned;
}

void save_texture_png( Texture const&  tx,
                       fs::path const& file ) {
  logger->info( "writing png file {}", file.string() );
  set_render_target( tx );
  auto           delta   = texture_delta( tx );
  ::SDL_Surface* surface = SDL_CreateRGBSurface(
      0, delta.w._, delta.h._, 32, 0, 0, 0, 0 );
  ::SDL_RenderReadPixels( g_renderer, NULL, g_pixel_format,
                          surface->pixels, surface->pitch );
  CHECK( !::IMG_SavePNG( surface, file.string().c_str() ),
         "failed to save png file {}", file.string() );
  ::SDL_FreeSurface( surface );
}

void grab_screen( fs::path const& file ) {
  auto screen = main_window_logical_size();
  logger->info(
      "grabbing screen with size [{} x {}] and saving to {}",
      screen.w, screen.h, file.string() );
  ::SDL_Surface* surface = create_surface( screen );
  set_render_target( Texture{} );
  ::SDL_RenderReadPixels( g_renderer, NULL, g_pixel_format,
                          surface->pixels, surface->pitch );
  CHECK( !::IMG_SavePNG( surface, file.string().c_str() ),
         "failed to save png file {}", file.string() );
  ::SDL_FreeSurface( surface );
}

void clear_texture_black( Texture const& tx ) {
  set_render_target( tx );
  ::SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, 255 );
  ::SDL_RenderClear( g_renderer );
}

void clear_texture_transparent( Texture const& tx ) {
  set_render_target( tx );
  ::SDL_SetTextureBlendMode( tx, ::SDL_BLENDMODE_NONE );
  ::SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, 0 );
  ::SDL_RenderClear( g_renderer );
  // TODO: this shouldn't be necessary since anyone who is
  // relying on blend mode should be setting it prior to
  // doing any operations.
  ::SDL_SetTextureBlendMode( tx, ::SDL_BLENDMODE_BLEND );
}

// TODO: mac-os, does not seem to be able to detect when the user
// fullscreens a window.
bool is_window_fullscreen() {
  // This bit should always be set even if we're in the "desktop"
  // fullscreen mode.
  return ( ::SDL_GetWindowFlags( g_window ) &
           ::SDL_WINDOW_FULLSCREEN ) != 0;
}

void set_fullscreen( bool fullscreen ) {
  bool already = is_window_fullscreen();
  if( ( fullscreen ^ already ) == 0 ) return;

  // Must only contain one of the following values.
  ::Uint32 flags =
      fullscreen ? ::SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
  ::SDL_SetWindowFullscreen( g_window, flags );
}

bool toggle_fullscreen() {
  auto fullscreen = is_window_fullscreen();
  set_fullscreen( !fullscreen );
  return !fullscreen;
}

void restore_window() { ::SDL_RestoreWindow( g_window ); }

Texture::Texture( ::SDL_Texture* tx )
  : own_{true}, tx_( tx ), id_{g_next_texture_id++} {
  CHECK( tx_ );
}

Texture::Texture( Texture&& tx ) noexcept
  : own_{false}, tx_( tx.tx_ ), id_( tx.id_ ) {
  tx.own_ = false;
  tx.tx_  = nullptr;
  tx.id_  = 0;
}

Texture& Texture::operator=( Texture&& rhs ) noexcept {
  if( own_ && tx_ != nullptr ) ::SDL_DestroyTexture( tx_ );
  tx_      = rhs.tx_;
  own_     = rhs.own_;
  id_      = rhs.id_;
  rhs.tx_  = nullptr;
  rhs.own_ = false;
  rhs.id_  = 0;
  return *this;
}

void Texture::free() {
  if( own_ && tx_ != nullptr ) ::SDL_DestroyTexture( tx_ );
  own_ = false;
  tx_  = nullptr;
  id_  = 0;
}

Texture::~Texture() { free(); }

Texture Texture::weak_ref() const {
  Texture res;
  res.own_ = false;
  res.tx_  = tx_;
  res.id_  = id_;
  return res;
}

Texture Texture::from_surface( ::SDL_Surface* surface ) {
  auto* optimized =
      optimize_surface( surface, /*release_input=*/false );
  ASSIGN_CHECK( texture, ::SDL_CreateTextureFromSurface(
                             g_renderer, optimized ) );
  ::SDL_FreeSurface( optimized );
  return from_SDL( texture );
}

Delta Texture::size() const {
  int w, h;
  ::SDL_QueryTexture( this->get(), nullptr, nullptr, &w, &h );
  return {W( w ), H( h )};
}

double Texture::mem_usage_mb( Delta size ) {
  // Not certain how to know memory usage of a texture, and it
  // may be device dependent. Found a formula online that adds a
  // 1.33 factor in there.  Note: 4 bytes per pixel.
  return size.w._ * size.h._ * 4 * 1.33 / ( 1024 * 1024 );
}

double Texture::mem_usage_mb() const {
  return Texture::mem_usage_mb( size() );
}

::SDL_Color color_from_pixel( SDL_PixelFormat* fmt,
                              Uint32           pixel ) {
  CHECK( fmt->BitsPerPixel == 32, "bits per pixel: {}",
         fmt->BitsPerPixel );
  ::SDL_Color color{};

  /* Get Red component */
  auto temp = pixel & fmt->Rmask;  /* Isolate red component */
  temp      = temp >> fmt->Rshift; /* Shift it down to 8-bit */
  temp = temp << fmt->Rloss; /* Expand to a full 8-bit number */
  color.r = (Uint8)temp;

  /* Get Green component */
  temp = pixel & fmt->Gmask;  /* Isolate green component */
  temp = temp >> fmt->Gshift; /* Shift it down to 8-bit */
  temp = temp << fmt->Gloss;  /* Expand to a full 8-bit number */
  color.g = (Uint8)temp;

  /* Get Blue component */
  temp = pixel & fmt->Bmask;  /* Isolate blue component */
  temp = temp >> fmt->Bshift; /* Shift it down to 8-bit */
  temp = temp << fmt->Bloss;  /* Expand to a full 8-bit number */
  color.b = (Uint8)temp;

  /* Get Alpha component */
  temp = pixel & fmt->Amask;  /* Isolate alpha component */
  temp = temp >> fmt->Ashift; /* Shift it down to 8-bit */
  temp = temp << fmt->Aloss;  /* Expand to a full 8-bit number */
  color.a = (Uint8)temp;

  return color;
}

::SDL_Color to_SDL( Color color ) {
  return {color.r, color.g, color.b, color.a};
}

Color from_SDL( ::SDL_Color color ) {
  return {color.r, color.g, color.b, color.a};
}

void set_render_draw_color( Color color ) {
  CHECK( !::SDL_SetRenderDrawColor( g_renderer, color.r, color.g,
                                    color.b, color.a ) );
}

void render_fill_rect( Texture const& tx, Color color,
                       Rect const& rect ) {
  set_render_target( move( tx ) );
  set_render_draw_color( color );
  auto sdl_rect = to_SDL( rect );
  ::SDL_RenderFillRect( g_renderer, &sdl_rect );
}

auto rounded_corner_template( rounded_corner_type type,
                              Color               color ) {
  switch( type ) {
    case rounded_corner_type::radius_2: {
      /*
       *  XXYOOOOO
       *  XOOOOOOO
       *  YOOOOOOO
       *  OOOOOOOO
       *  OOOOOOOO
       */
      auto faded = color;
      faded.a /= 4;
      faded.a *= 3;
      return vector<Pixel>{
          {{2_x, 1_y}, color}, {{3_x, 1_y}, color},
          {{1_x, 2_y}, color}, {{2_x, 2_y}, color},
          {{3_x, 2_y}, color}, {{1_x, 3_y}, color},
          {{2_x, 3_y}, color}, {{3_x, 3_y}, color},
          {{3_x, 0_y}, color}, {{0_x, 3_y}, color},
          {{1_x, 1_y}, color}, {{2_x, 0_y}, faded},
          {{0_x, 2_y}, faded}};
    }
    case rounded_corner_type::radius_3: {
      /*
       *  XXZYOOOO
       *  XYOOOOOO
       *  ZOOOOOOO
       *  YOOOOOOO
       *  OOOOOOOO
       */
      auto faded = color;
      faded.a /= 4;
      faded.a *= 3;
      auto faded2 = color;
      faded2.a /= 4;
      faded2.a /= 2;
      return vector<Pixel>{
          {{2_x, 1_y}, color}, {{3_x, 1_y}, color},
          {{1_x, 2_y}, color}, {{2_x, 2_y}, color},
          {{3_x, 2_y}, color}, {{1_x, 3_y}, color},
          {{2_x, 3_y}, color}, {{3_x, 3_y}, color},
          {{3_x, 0_y}, faded}, {{0_x, 3_y}, faded},
          {{1_x, 1_y}, faded}, {{2_x, 0_y}, faded2},
          {{0_x, 2_y}, faded2}};
    }
    case rounded_corner_type::radius_4: {
      /*
       *  XXXXOOOO
       *  XXOOOOOO
       *  XOOOOOOO
       *  XOOOOOOO
       *  OOOOOOOO
       */
      auto faded = color;
      faded.a /= 3;
      faded.a *= 2;
      return vector<Pixel>{
          {{2_x, 1_y}, color}, {{3_x, 1_y}, color},
          {{1_x, 2_y}, color}, {{2_x, 2_y}, color},
          {{3_x, 2_y}, color}, {{1_x, 3_y}, color},
          {{2_x, 3_y}, color}, {{3_x, 3_y}, color}};
    }
  }
  SHOULD_NOT_BE_HERE;
  return vector<Pixel>{}; // for gcc
}

// WARNING: this is slow, only use in pre-rendered textures.
void render_fill_rect_rounded( Texture const& tx, Color color,
                               Rect const&         rect,
                               rounded_corner_type type ) {
  SHOULD_BE_HERE_ONLY_DURING_INITIALIZATION;
  ::SDL_SetRenderDrawBlendMode( g_renderer,
                                ::SDL_BLENDMODE_NONE );
  auto template_points = rounded_corner_template( type, color );
  vector<Pixel> points;
  for( auto pixel : template_points ) {
    auto delta    = pixel.coord - Coord{};
    auto delta_ll = delta;
    delta_ll.h    = -delta_ll.h - 1_h;
    auto delta_ur = delta;
    delta_ur.w    = -delta_ur.w - 1_w;
    points.push_back( {rect.upper_left() + delta, pixel.color} );
    points.push_back(
        {rect.lower_right() - delta - Delta{1_w, 1_h},
         pixel.color} );
    points.push_back(
        {rect.lower_left() + delta_ll, pixel.color} );
    points.push_back(
        {rect.upper_right() + delta_ur, pixel.color} );
  }
  util::sort_by_key( points, L( _.color ) );
  for( auto rng :
       points | rv::group_by( L2( _1.color == _2.color ) ) ) {
    vector<Coord> coords = rng | rv::transform( L( _.coord ) );
    CHECK( coords.size() > 0 );
    Color c = rng.begin()->color;
    render_points( tx, c, coords );
  }

  auto left_middle = rect;
  left_middle.w    = 4_w;
  left_middle.h -= 8_h;
  left_middle.y += 4_h;
  render_fill_rect( tx, color, left_middle );

  auto top_middle = rect;
  top_middle.w -= 8_w;
  top_middle.h = 4_h;
  top_middle.x += 4_w;
  render_fill_rect( tx, color, top_middle );

  auto right_middle = rect;
  right_middle.w    = 4_w;
  right_middle.h -= 8_h;
  right_middle.y += 4_h;
  right_middle.x += ( rect.w - 4_w );
  render_fill_rect( tx, color, right_middle );

  auto bottom_middle = rect;
  bottom_middle.w -= 8_w;
  bottom_middle.h = 4_h;
  bottom_middle.y += ( rect.h - 4_h );
  bottom_middle.x += 4_w;
  render_fill_rect( tx, color, bottom_middle );

  auto middle = rect;
  middle.w -= 8_w;
  middle.h -= 8_h;
  middle.y += 4_h;
  middle.x += 4_w;
  render_fill_rect( tx, color, middle );

  ::SDL_SetRenderDrawBlendMode( g_renderer,
                                ::SDL_BLENDMODE_BLEND );
}

void fill_texture( Texture const& tx, Color color ) {
  render_fill_rect( tx, color,
                    Rect::from( Coord{}, texture_delta( tx ) ) );
}

void render_line( Texture const& tx, Color color, Coord start,
                  Delta delta ) {
  // The SDL rendering method used below includes end points, so
  // we must avoid calling it if the line will have zero length.
  if( delta == Delta::zero() ) return;
  set_render_target( tx );
  set_render_draw_color( color );
  Coord end = start + delta.trimmed_by_one();
  ::SDL_RenderDrawLine( g_renderer, start.x._, start.y._,
                        end.x._, end.y._ );
}

void render_rect( Texture const& tx, Color color,
                  Rect const& rect ) {
  set_render_target( move( tx ) );
  set_render_draw_color( color );
  auto sdl_rect = to_SDL( rect );
  ::SDL_RenderDrawRect( g_renderer, &sdl_rect );
}

// Caller must set blend mode!
void render_points( Texture const& tx, Color color,
                    vector<Coord> const& points ) {
  auto to_sdl = []( Coord const& coord ) {
    return to_SDL( coord );
  };
  auto sdl_points = util::map( to_sdl, points );
  set_render_target( tx );
  set_render_draw_color( color );
  CHECK( !::SDL_RenderDrawPoints( g_renderer, &sdl_points[0],
                                  sdl_points.size() ) );
}

} // namespace rn
