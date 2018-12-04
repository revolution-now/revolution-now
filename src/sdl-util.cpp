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
#include <iostream>
#include <vector>

#include "sdl-util.hpp"

#include "errors.hpp"
#include "fonts.hpp"
#include "global-constants.hpp"
#include "globals.hpp"
#include "sound.hpp"
#include "util.hpp"

#include <SDL_mixer.h>

#include <cmath>
#include <iomanip>
#include <sstream>

using namespace std;

ostream& operator<<( ostream& out, ::SDL_Rect const& r ) {
  return ( out << "(" << r.x << "," << r.y << "," << r.w << ","
               << r.h << ")" );
}

namespace rn {

namespace {

vector<Texture> loaded_textures;

ostream& operator<<( ostream&                 out,
                     ::SDL_DisplayMode const& dm ) {
  return ( out << dm.w << "x" << dm.h << "[" << dm.refresh_rate
               << "Hz]" );
}

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

ND Rect from_SDL( ::SDL_Rect const& rect ) {
  Rect res;
  res.x = rect.x;
  res.y = rect.y;
  res.w = rect.w;
  res.h = rect.h;
  return res;
}

void init_game() {
  rn::init_sdl();
  rn::init_fonts();
  rn::create_window();
  rn::print_video_stats();
  rn::create_renderer();
}

void init_sdl() {
  if( ::SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
    DIE( "sdl could not initialize" );

  constexpr int frequency{44100};
  constexpr int chunksize{2048};

  // Open Audio device
  if( Mix_OpenAudio( frequency, AUDIO_S16SYS, 2, chunksize ) !=
      0 ) {
    cerr << "Mix_OpenAudio ERROR: " << ::Mix_GetError() << endl;
    DIE( "could not open audio" );
  }
  // Set Volume
  constexpr int default_volume{10};

  ::Mix_VolumeMusic( default_volume );
}

SDL_DisplayMode find_fullscreen_mode() {
  ::SDL_DisplayMode dm;
  cout << "Available display modes:\n";
  auto num_display_modes = ::SDL_GetNumDisplayModes( 0 );
  constexpr int min_x_res{1920};
  constexpr int min_y_res{1080};
  for( int i = 0; i < num_display_modes; ++i ) {
    ::SDL_GetDisplayMode( 0, i, &dm );
    if( dm.w % g_tile_width._ == 0 &&
        dm.h % g_tile_height._ == 0 ) {
      cout << dm.w << "x" << dm.h << "\n";
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
  // if( !fullscreen_mode.w )
  //  DIE( "cannot find appropriate fullscreen mode" );

  auto dm = get_current_display_mode();

  g_window =
      ::SDL_CreateWindow( string( g_window_title ).c_str(), 0, 0,
                          dm.w, dm.h, flags );
  if( g_window == nullptr ) DIE( "failed to create window" );

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
  cout << "GetDisplayDPI:\n";
  cout << "  ddpi: " << ddpi << "\n";
  cout << "  hdpi: " << hdpi << "\n";
  cout << "  vdpi: " << vdpi << "\n";

  SDL_DisplayMode dm;

  cout << "GetCurrentDisplayMode:\n";
  SDL_GetCurrentDisplayMode( 0, &dm );
  cout << "  " << dm << "\n";

  cout << "GetDesktopDisplayMode:\n";
  SDL_GetDesktopDisplayMode( 0, &dm );
  cout << "  " << dm << "\n";

  cout << "GetDisplayMode:\n";
  SDL_GetDisplayMode( 0, 0, &dm );
  cout << "  " << dm << "\n";

  SDL_Rect r;
  cout << "GetDisplayBounds:\n";
  SDL_GetDisplayBounds( 0, &r );
  cout << "  " << r << "\n";

  cout << "Monitor Diagonal Length: " << monitor_inches()
       << "in."
       << "\n";
}

pair<H, W> find_max_tile_sizes() {
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
  constexpr double ideal_tile_size_at_1ft{.25}; // inches
  auto compute_viewer_distance = []( double monitor_size ) {
    // Determined empirically; viewer distance from screen seems
    // to scale linearly with screen size, down to a certain
    // minimum distance.
    constexpr double viewer_distance_multiplier{1.25};
    constexpr double viewer_distance_minimum{18}; // inches
    return max( viewer_distance_multiplier * monitor_size,
                viewer_distance_minimum );
  };
  // Seems sensible for the tiles to be within these bounds.  The
  // scoring function used would otherwise select these in some
  // cases
  constexpr double minimum_perceived_tile_width{0.2}; // inches
  constexpr double maximum_perceived_tile_width{1.0}; // inches
  ////////////////////////////////////////////////

  cout << "Finding optimal tile sizes:\n";
  auto                 dm = get_current_display_mode();
  optional<pair<H, W>> res;
  double               min_score = -1.0 / 0.0; // make -infinity
  constexpr int        col{18};
  double               monitor_size = monitor_inches();
  cout << "Computed Viewer Distance from Screen: "
       << compute_viewer_distance( monitor_size ) << "in.\n";
  double ddpi = monitor_ddpi();
  cout << "\n  " << setw( 3 ) << "#" << setw( col )
       << "Possibility" << setw( col ) << "Resolution"
       << setw( col ) << "Tile-Size-Screen" << setw( col )
       << "Tile-Size-@1ft" << setw( col ) << "Score"
       << "\n";
  int chosen_scale = -1;

  constexpr int max_scale{10}; // somewhat arbitrary
  for( int scale = 1; scale <= max_scale; ++scale ) {
    cout << "  " << setw( 3 ) << scale;

    W max_width{dm.w / scale -
                ( ( dm.w / scale ) % g_tile_width._ )};
    H max_height{dm.h / scale -
                 ( ( dm.h / scale ) % g_tile_height._ )};

    ostringstream ss;
    ss << max_width / g_tile_width << "x"
       << max_height / g_tile_height;
    cout << setw( col ) << ss.str();
    ss.str( "" );
    ss << max_width << "x" << max_height;
    cout << setw( col ) << ss.str();

    // Tile size in inches if it were measured on the surface of
    // the screen.
    double tile_size_actual = scale * g_tile_width._ / ddpi;
    cout << setw( col ) << tile_size_actual;
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
    cout << setw( col ) << perceived_size_1ft;

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
    cout << setw( col ) << score;
    cout << "\n";
    if( score >= min_score ) {
      res = {H( max_height / g_tile_height ),
             W( max_width / g_tile_width )};

      min_score    = score;
      chosen_scale = scale;
    }
  }
  CHECK( res );
  cout << "\n  Optimal: #" << chosen_scale << "\n";
  return *res;
}

void create_renderer() {
  g_renderer = SDL_CreateRenderer(
      g_window, -1,
      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );

  if( g_renderer == nullptr ) DIE( "failed to create renderer" );

  auto screen_sizes = find_max_tile_sizes();
  set_screen_width_tiles( screen_sizes.second );
  set_screen_height_tiles( screen_sizes.first );

  W width  = g_tile_width._ * screen_width_tiles();
  H height = g_tile_height._ * screen_height_tiles();
  // cout << "logical renderer width : " << width << "\n";
  // cout << "logical renderer height: " << height << "\n";

  ::SDL_RenderSetLogicalSize( g_renderer, width._, height._ );
  ::SDL_RenderSetIntegerScale( g_renderer, ::SDL_TRUE );
  ::SDL_SetRenderDrawBlendMode( g_renderer,
                                ::SDL_BLENDMODE_BLEND );

  // +1 tile because we may need to draw a bit in excess of the
  // viewport window in order to facilitate smooth scrolling,
  // though we shouldn't need more than 1 extra tile for that
  // purpose.  The *2 is so that we can accomodate the maximum
  // zoomed-out level which is .5.
  width  = g_tile_width._ * 2 * ( viewport_width_tiles() + 1 );
  height = g_tile_height._ * 2 * ( viewport_height_tiles() + 1 );
  g_texture_world = create_texture( width, height );
}

Texture from_SDL( ::SDL_Texture* tx ) { return Texture( tx ); }

Texture& load_texture( const char* file ) {
  SDL_Surface* pTempSurface = IMG_Load( file );
  if( pTempSurface == nullptr ) DIE( "failed to load image" );
  ::SDL_Texture* texture = SDL_CreateTextureFromSurface(
      rn::g_renderer, pTempSurface );
  if( texture == nullptr ) DIE( "failed to create texture" );
  SDL_FreeSurface( pTempSurface );
  loaded_textures.emplace_back( from_SDL( texture ) );
  return loaded_textures.back();
}

// All the functions in this method should not cause problems
// even if their corresponding initialization routines were
// not successfully run.
void cleanup() {
  unload_fonts();
  cleanup_sound();
  if( g_renderer != nullptr ) SDL_DestroyRenderer( g_renderer );
  if( g_window != nullptr ) SDL_DestroyWindow( g_window );
  // Not clear if this actually quits the program; but it does
  // deinitialize everything.
  SDL_Quit();
}

Rect texture_rect( Texture const& texture ) {
  int w, h;
  // const_cast because we are passing texture to a C function
  // which we know will not modify it.
  ::SDL_QueryTexture( texture, nullptr, nullptr, &w, &h );
  return {X( 0 ), Y( 0 ), W( w ), H( h )};
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

bool copy_texture( Texture const& from, OptCRef<Texture> to, Y y,
                   X x ) {
  ::SDL_Texture* target = to ? ( *to ).get().get() : nullptr;
  ::SDL_SetTextureBlendMode( from, ::SDL_BLENDMODE_BLEND );
  ::SDL_SetTextureBlendMode( target, ::SDL_BLENDMODE_BLEND );
  if( ::SDL_SetRenderTarget( g_renderer, target ) != 0 )
    return false;
  Rect dest     = texture_rect( from );
  dest.x        = x;
  dest.y        = y;
  auto sdl_rect = to_SDL( dest );
  return ::SDL_RenderCopy( g_renderer, from, nullptr,
                           &sdl_rect ) == 0;
}

bool copy_texture( Texture const& from, OptCRef<Texture> to,
                   Coord const& coord ) {
  return copy_texture( from, move( to ), coord.y, coord.x );
}

Texture create_texture( W w, H h ) {
  auto tx = from_SDL( ::SDL_CreateTexture(
      g_renderer, SDL_PIXELFORMAT_RGBA8888,
      SDL_TEXTUREACCESS_TARGET, w._, h._ ) );
  clear_texture_black( tx );
  return tx;
}

void clear_texture_black( Texture const& tx ) {
  ::SDL_SetRenderTarget( g_renderer, tx );
  ::SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, 0 );
  ::SDL_RenderClear( g_renderer );
}

void render_texture( Texture const& texture, SDL_Rect source,
                     SDL_Rect dest, double angle,
                     SDL_RendererFlip flip ) {
  SDL_Rect dest_shifted = dest;
  if( SDL_RenderCopyEx( g_renderer, texture, &source,
                        &dest_shifted, angle, nullptr,
                        flip ) > 0 )
    DIE( "failed to render texture" );
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

Texture::~Texture() {
  if( tx_ != nullptr ) ::SDL_DestroyTexture( tx_ );
}

Texture Texture::from_surface( ::SDL_Surface* surface ) {
  ASSIGN_CHECK( texture, ::SDL_CreateTextureFromSurface(
                             g_renderer, surface ) );
  return from_SDL( texture );
}

void set_render_draw_color( Color const& color ) {
  CHECK( !::SDL_SetRenderDrawColor( g_renderer, color.r, color.g,
                                    color.b, color.a ) );
}

void render_fill_rect( OptCRef<Texture> tx, Color const& color,
                       Rect const& rect ) {
  set_render_target( move( tx ) );
  set_render_draw_color( color );
  auto sdl_rect = to_SDL( rect );
  ::SDL_RenderFillRect( g_renderer, &sdl_rect );
}

void render_rect( OptCRef<Texture> tx, Color const& color,
                  Rect const& rect ) {
  set_render_target( move( tx ) );
  set_render_draw_color( color );
  auto sdl_rect = to_SDL( rect );
  ::SDL_RenderDrawRect( g_renderer, &sdl_rect );
}

} // namespace rn
