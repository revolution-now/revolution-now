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
#include "fonts.hpp"
#include "globals.hpp"
#include "image.hpp"
#include "logging.hpp"
#include "menu.hpp"
#include "plane.hpp"
#include "sound.hpp"
#include "tiles.hpp"
#include "util.hpp"

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

SDL_DisplayMode get_current_display_mode() {
  SDL_DisplayMode dm;
  SDL_GetCurrentDisplayMode( 0, &dm );
  return dm;
}

vector<Rect> clip_stack;

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

void init_game() {
  logger->info( "Initializing SDL" );
  init_sdl();
  logger->info( "Initializing fonts" );
  init_fonts();
  logger->info( "Initializing SDL window" );
  create_window();
  print_video_stats();
  logger->info( "Initializing global renderer" );
  create_renderer();
  logger->info( "Initializing planes" );
  initialize_planes();
  logger->info( "Loading sound effects" );
  load_all_sfx();
  logger->info( "Loading images" );
  load_all_images();
  logger->info( "Initializing menus" );
  initialize_menus();
}

void init_sdl() {
  CHECK( ::SDL_Init( SDL_INIT_EVERYTHING ) >= 0,
         "sdl could not initialize" );

  constexpr int frequency{44100};
  constexpr int chunksize{4096};
  constexpr int channels{2};

  // Open Audio device
  CHECK( !Mix_OpenAudio( frequency, AUDIO_S16SYS, channels,
                         chunksize ),
         "could not open audio: Mix_OpenAudio ERROR: {}",
         ::Mix_GetError() );

  // Set Volume
  constexpr int default_volume{10};

  ::Mix_VolumeMusic( default_volume );
}

SDL_DisplayMode find_fullscreen_mode() {
  ::SDL_DisplayMode dm;
  LOG_DEBUG( "Available display modes:" );
  auto num_display_modes = ::SDL_GetNumDisplayModes( 0 );
  constexpr int min_x_res{1920};
  constexpr int min_y_res{1080};
  for( int i = 0; i < num_display_modes; ++i ) {
    ::SDL_GetDisplayMode( 0, i, &dm );
    if( dm.w % g_tile_width._ == 0 &&
        dm.h % g_tile_height._ == 0 ) {
      LOG_DEBUG( "{}x{}", dm.w, dm.h );
      if( dm.w >= min_x_res && dm.h >= min_y_res ) return dm;
    }
  }
  dm.w = dm.h = 0; // means we can't find one.
  return dm;
}

void create_window() {
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  auto flags = ::SDL_WINDOW_SHOWN | ::SDL_WINDOW_RESIZABLE |
               ::SDL_WINDOW_FULLSCREEN_DESKTOP;

  // auto fullscreen_mode = find_fullscreen_mode();
  // CHECK( fullscreen_mode.w,
  //  "cannot find appropriate fullscreen mode" );

  auto dm = get_current_display_mode();

  g_window =
      ::SDL_CreateWindow( config_rn.main_window.title.c_str(), 0,
                          0, dm.w, dm.h, flags );
  CHECK( g_window != nullptr, "failed to create window" );

  //::SDL_SetWindowDisplayMode( g_window, &fullscreen_mode );
}

double monitor_diagonal_length( float             ddpi,
                                ::SDL_DisplayMode dm ) {
  double length =
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
      sqrt( pow( dm.w, 2.0 ) + pow( dm.h, 2.0 ) ) / ddpi;
  // Round to hearest 1/2 inch.
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
  return double( lround( length * 2.0 ) ) / 2.0;
}

// Get diagonal DPI of monitor. These seems to basically be the
// same as the horizontal and vertical DPIs.
double monitor_ddpi() {
  float ddpi;
  CHECK( !::SDL_GetDisplayDPI( 0, &ddpi, nullptr, nullptr ) );
  return ddpi;
}

double monitor_inches() {
  float           ddpi = monitor_ddpi();
  SDL_DisplayMode dm;
  SDL_GetCurrentDisplayMode( 0, &dm );
  return monitor_diagonal_length( ddpi, dm );
}

void print_video_stats() {
  float ddpi, hdpi, vdpi;
  ::SDL_GetDisplayDPI( 0, &ddpi, &hdpi, &vdpi );
  LOG_DEBUG( "GetDisplayDPI:" );
  LOG_DEBUG( "  ddpi: {}", ddpi );
  LOG_DEBUG( "  hdpi: {}", hdpi );
  LOG_DEBUG( "  vdpi: {}", vdpi );

  SDL_DisplayMode dm;

  auto dm_to_str = [&dm] {
    return fmt::format( "{}x{}[{}Hz]", dm.w, dm.h,
                        dm.refresh_rate );
  };
  (void)dm_to_str;

  LOG_DEBUG( "GetCurrentDisplayMode: " );
  SDL_GetCurrentDisplayMode( 0, &dm );
  LOG_DEBUG( "  {}", dm_to_str() );

  LOG_DEBUG( "GetDesktopDisplayMode: " );
  SDL_GetDesktopDisplayMode( 0, &dm );
  LOG_DEBUG( "  {}", dm_to_str() );

  LOG_DEBUG( "GetDisplayMode: " );
  SDL_GetDisplayMode( 0, 0, &dm );
  LOG_DEBUG( "  {}", dm_to_str() );

  SDL_Rect r;
  LOG_DEBUG( "GetDisplayBounds:" );
  SDL_GetDisplayBounds( 0, &r );
  LOG_DEBUG( "  {}", from_SDL( r ) );

  LOG_DEBUG( "Monitor Diagonal Length: {}in.",
             monitor_inches() );
}

void find_max_tile_sizes() {
  // We want ideally to have a tile whose side is this length in
  // inches. The algorithm that follows will try to find an
  // integer scaling factor (and associated maximal screen width
  // and height in tiles) such that a) tiles are square, tiles
  // side lengths are as close as possible to this length in
  // inches, and c) as much of the screen is covered as possible.
  // Note that we have opted to avoid scaling the tile grid by
  // non-integer values, and so that represents a key constraint
  // here. Hence we won't generally achieve the ideal tile size,
  // but should come close to it.
  constexpr double ideal_tile_size_at_1ft{.30}; // inches
  auto compute_viewer_distance = []( double monitor_size ) {
    // Determined empirically; viewer distance from screen seems
    // to scale linearly with screen size, down to a certain
    // minimum distance.
    constexpr double viewer_distance_multiplier{1.25};
    constexpr double viewer_distance_minimum{18}; // inches
    return std::max( viewer_distance_multiplier * monitor_size,
                     viewer_distance_minimum );
  };
  // Seems sensible for the tiles to be within these bounds.  The
  // scoring function used would otherwise select these in some
  // cases
  constexpr double minimum_perceived_tile_width{0.2}; // inches
  constexpr double maximum_perceived_tile_width{1.0}; // inches
  ////////////////////////////////////////////////

  // cout << "Finding optimal tile sizes:\n";
  auto            dm = get_current_display_mode();
  optional<Delta> result;
  double          min_score = -1.0 / 0.0; // make -infinity
  // constexpr int        col{18};
  double monitor_size = monitor_inches();
  // cout << "Computed Viewer Distance from Screen: "
  //     << compute_viewer_distance( monitor_size ) << "in.\n";
  double ddpi = monitor_ddpi();
  // cout << "\n  " << setw( 3 ) << "#" << setw( col )
  //     << "Possibility" << setw( col ) << "Resolution"
  //     << setw( col ) << "Tile-Size-Screen" << setw( col )
  //     << "Tile-Size-@1ft" << setw( col ) << "Score"
  //     << "\n";
  int chosen_scale = -1;

  constexpr int max_scale{10}; // somewhat arbitrary
  for( int scale = 1; scale <= max_scale; ++scale ) {
    // cout << "  " << setw( 3 ) << scale;

    W max_width{dm.w / scale -
                ( ( dm.w / scale ) % g_tile_width._ )};
    H max_height{dm.h / scale -
                 ( ( dm.h / scale ) % g_tile_height._ )};

    // ostringstream ss;
    // ss << max_width / g_tile_width << "x"
    //   << max_height / g_tile_height;
    // cout << setw( col ) << ss.str();
    // ss.str( "" );
    // ss << max_width << "x" << max_height;
    // cout << setw( col ) << ss.str();

    // Tile size in inches if it were measured on the surface of
    // the screen.
    double tile_size_actual = scale * g_tile_width._ / ddpi;
    // cout << setw( col ) << tile_size_actual;
    // Estimate the viewer's distance from the screen based on
    // its size and some other assumptions.
    double viewer_distance =
        compute_viewer_distance( monitor_size );
    constexpr double one_foot{12.0};
    // This is the apparent size in inches of a tile when it is
    // measure by a ruler that is placed one foot in front of the
    // viewer's eye.
    double perceived_size_1ft =
        tile_size_actual / viewer_distance * one_foot;
    // cout << setw( col ) << perceived_size_1ft;

    // Essentially this gives less weight the further we move
    // away from the ideal.
    double score =
        -::abs( perceived_size_1ft - ideal_tile_size_at_1ft );
    // If the tile size is smaller than a minimum cutoff then
    // we will avoid selecting it by giving it a score smaller
    // than any other.
    if( perceived_size_1ft < minimum_perceived_tile_width ||
        perceived_size_1ft > maximum_perceived_tile_width )
      score = -1.0 / 0.0;
    // cout << setw( col ) << score;
    // cout << "\n";
    if( score >= min_score ) {
      result = {W( max_width / g_tile_width ),
                H( max_height / g_tile_height )};

      min_score    = score;
      chosen_scale = scale;
    }
  }
  CHECK( result, "Could not find a suitable scaling" );
  auto const& delta = *result;
  logger->debug( "Optimal scale factor: #{}", chosen_scale );
  set_screen_width_tiles( delta.w );
  set_screen_height_tiles( delta.h );
  logger->debug( "tiles: {} x {}", delta.w, delta.h );
  g_resolution_scale_factor = Scale( chosen_scale );
  g_drawing_origin.w =
      ( dm.w -
        ( chosen_scale * ( delta.w * g_tile_width )._ ) ) /
      2;
  g_drawing_origin.h =
      ( dm.h -
        ( chosen_scale * ( delta.h * g_tile_height )._ ) ) /
      2;
  g_drawing_region.x = 0_x + g_drawing_origin.w;
  g_drawing_region.y = 0_y + g_drawing_origin.h;
  g_drawing_region.w = dm.w - g_drawing_origin.w * 2_sx;
  g_drawing_region.h = dm.h - g_drawing_origin.h * 2_sy;
  logger->debug( "w drawing region: {}", g_drawing_region );
  logger->debug( "logical screen pixel dimensions: {}",
                 logical_screen_pixel_dimensions() );
}

void create_renderer() {
  g_renderer = SDL_CreateRenderer(
      g_window, -1,
      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );

  CHECK( g_renderer, "failed to create renderer" );

  find_max_tile_sizes();

  W width  = screen_width_tiles() * g_tile_width;
  H height = screen_height_tiles() * g_tile_height;

  ::SDL_RenderSetLogicalSize( g_renderer, width._, height._ );
  // Don't need this since we impose the integer scaling manually
  //::SDL_RenderSetIntegerScale( g_renderer, ::SDL_TRUE );
  ::SDL_SetRenderDrawBlendMode( g_renderer,
                                ::SDL_BLENDMODE_BLEND );

  logger->debug( "screen_logical_size(): {}",
                 screen_logical_size() );

  // +1 tile because we may need to draw a bit in excess of the
  // viewport window in order to facilitate smooth scrolling,
  // though we shouldn't need more than 1 extra tile for that
  // purpose.  The *2 is so that we can accomodate the maximum
  // zoomed-out level which is .5.
  width = ( viewport_width_tiles() + 1 ) * 2_sx * g_tile_width;
  height =
      ( viewport_height_tiles() + 1 ) * 2_sy * g_tile_height;
  g_texture_viewport = create_texture( width, height );
}

Texture from_SDL( ::SDL_Texture* tx ) { return Texture( tx ); }

::SDL_Surface* load_surface( const char* file ) {
  SDL_Surface* surface = IMG_Load( file );
  CHECK( surface, "failed to load image: {}", file );
  return surface;
}

Texture& load_texture( const char* file ) {
  SDL_Surface* pTempSurface = IMG_Load( file );
  CHECK( pTempSurface != nullptr, "failed to load image" );
  ::SDL_Texture* texture =
      SDL_CreateTextureFromSurface( g_renderer, pTempSurface );
  CHECK( texture != nullptr, "failed to create texture" );
  SDL_FreeSurface( pTempSurface );
  loaded_textures[string( file )] = from_SDL( texture );
  return loaded_textures[string( file )];
}

Texture& load_texture( fs::path const& path ) {
  return load_texture( path.string().c_str() );
}

// All the functions in this method should not cause problems
// even if their corresponding initialization routines were
// not successfully run.
void cleanup() {
  cleanup_menus();
  destroy_planes();
  unload_fonts();
  cleanup_sound();
  for( auto& p : loaded_textures ) p.second.free();
  if( g_renderer != nullptr ) SDL_DestroyRenderer( g_renderer );
  if( g_window != nullptr ) SDL_DestroyWindow( g_window );
  // Not clear if this actually quits the program; but it does
  // deinitialize everything.
  SDL_Quit();
}

Delta texture_delta( Texture const& texture ) {
  int w, h;
  CHECK(
      !::SDL_QueryTexture( texture, nullptr, nullptr, &w, &h ) );
  return {W{w}, H{h}};
}

void set_render_target( OptCRef<Texture> tx ) {
  ::SDL_Texture* target = tx ? ( *tx ).get().get() : nullptr;
  CHECK( !::SDL_SetRenderTarget( g_renderer, target ) );
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

void copy_texture( Texture const& from, OptCRef<Texture> to,
                   Coord const& dst_coord ) {
  ::SDL_Texture* target = to ? ( *to ).get().get() : nullptr;
  ::SDL_SetTextureBlendMode( from, ::SDL_BLENDMODE_BLEND );
  ::SDL_SetTextureBlendMode( target, ::SDL_BLENDMODE_BLEND );
  set_render_target( to );
  auto rect     = Rect::from( dst_coord, texture_delta( from ) );
  auto sdl_rect = to_SDL( rect );
  CHECK( !::SDL_RenderCopy( g_renderer, from, nullptr,
                            &sdl_rect ) );
}

void copy_texture_to_main( Texture const& from ) {
  copy_texture( from, nullopt, Coord{} + g_drawing_origin );
}

void copy_texture( Texture const& from, Texture const& to ) {
  copy_texture( from, to, Coord{} );
}

void copy_texture_stretch( Texture const&   from,
                           OptCRef<Texture> to, Rect const& src,
                           Rect const& dest ) {
  ::SDL_Texture* target = to ? ( *to ).get().get() : nullptr;
  ::SDL_SetTextureBlendMode( from, ::SDL_BLENDMODE_BLEND );
  ::SDL_SetTextureBlendMode( target, ::SDL_BLENDMODE_BLEND );
  set_render_target( to );
  ::SDL_Rect sdl_src  = to_SDL( src );
  ::SDL_Rect sdl_dest = to_SDL( dest );
  CHECK( !::SDL_RenderCopy( g_renderer, from, &sdl_src,
                            &sdl_dest ) );
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

ND Texture create_screen_sized_texture() {
  return create_texture( screen_logical_size() );
}

::SDL_Surface* create_surface( Delta delta ) {
  SDL_Surface* surface = SDL_CreateRGBSurface(
      0, delta.w._, delta.h._, 32, 0, 0, 0, 0 );
  CHECK( surface != nullptr, "SDL_CreateRGBSurface failed" );
  return surface;
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

void save_texture_png( Texture const&  tx,
                       fs::path const& file ) {
  logger->info( "writing png file {}", file.string() );
  ::SDL_Texture* old = ::SDL_GetRenderTarget( g_renderer );
  ::SDL_SetRenderTarget( g_renderer, tx );
  auto           delta   = texture_delta( tx );
  ::SDL_Surface* surface = SDL_CreateRGBSurface(
      0, delta.w._, delta.h._, 32, 0, 0, 0, 0 );
  ::SDL_RenderReadPixels( g_renderer, NULL, g_pixel_format,
                          surface->pixels, surface->pitch );
  CHECK( !::IMG_SavePNG( surface, file.string().c_str() ),
         "failed to save png file {}", file.string() );
  ::SDL_FreeSurface( surface );
  ::SDL_SetRenderTarget( g_renderer, old );
}

Delta screen_logical_size() {
  //::SDL_SetRenderTarget( g_renderer, nullptr );
  // Delta screen;
  //::SDL_RenderGetLogicalSize( g_renderer, &screen.w._,
  //                            &screen.h._ );
  // return screen;
  return logical_screen_pixel_dimensions();
}

Rect screen_logical_rect() {
  return Rect::from( {0_y, 0_x}, screen_logical_size() );
}

void grab_screen( fs::path const& file ) {
  auto screen = screen_logical_size();
  logger->info(
      "grabbing screen with size [{} x {}] and saving to {}",
      screen.w, screen.h, file.string() );
  ::SDL_Surface* surface = create_surface( screen );
  set_render_target( nullopt );
  ::SDL_RenderReadPixels( g_renderer, NULL, g_pixel_format,
                          surface->pixels, surface->pitch );
  CHECK( !::IMG_SavePNG( surface, file.string().c_str() ),
         "failed to save png file {}", file.string() );
  ::SDL_FreeSurface( surface );
}

void clear_texture_black( Texture const& tx ) {
  ::SDL_SetRenderTarget( g_renderer, tx );
  ::SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, 255 );
  ::SDL_RenderClear( g_renderer );
}

void clear_texture_transparent( Texture const& tx ) {
  ::SDL_SetRenderTarget( g_renderer, tx );
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

void toggle_fullscreen() {
  set_fullscreen( !is_window_fullscreen() );
}

Texture::Texture( ::SDL_Texture* tx ) : tx_( tx ) {
  CHECK( tx_ );
}

Texture::Texture( Texture&& tx ) noexcept : tx_( tx.tx_ ) {
  tx.tx_ = nullptr;
}

Texture& Texture::operator=( Texture&& rhs ) noexcept {
  if( tx_ != nullptr ) ::SDL_DestroyTexture( tx_ );
  tx_     = rhs.tx_;
  rhs.tx_ = nullptr;
  return *this;
}

void Texture::free() {
  if( tx_ != nullptr ) ::SDL_DestroyTexture( tx_ );
  tx_ = nullptr;
}

Texture::~Texture() { free(); }

Texture Texture::from_surface( ::SDL_Surface* surface ) {
  ASSIGN_CHECK( texture, ::SDL_CreateTextureFromSurface(
                             g_renderer, surface ) );
  return from_SDL( texture );
}

Delta Texture::size() const {
  int w, h;
  ::SDL_QueryTexture( this->get(), nullptr, nullptr, &w, &h );
  return {W( w ), H( h )};
}

::SDL_Color color_from_pixel( SDL_PixelFormat* fmt,
                              Uint32           pixel ) {
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

void render_fill_rect( OptCRef<Texture> tx, Color color,
                       Rect const& rect ) {
  set_render_target( move( tx ) );
  set_render_draw_color( color );
  auto sdl_rect = to_SDL( rect );
  ::SDL_RenderFillRect( g_renderer, &sdl_rect );
}

void render_fill_rect( Texture const& tx, Color color ) {
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

void render_rect( OptCRef<Texture> tx, Color color,
                  Rect const& rect ) {
  set_render_target( move( tx ) );
  set_render_draw_color( color );
  auto sdl_rect = to_SDL( rect );
  ::SDL_RenderDrawRect( g_renderer, &sdl_rect );
}

} // namespace rn
