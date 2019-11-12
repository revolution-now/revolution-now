/****************************************************************
**tx.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-09.
*
* Description: Texture Wrapper.  This is to shield the rest of
* the codebase from the underlying texture representation as well
* as to provide RAII.
*
*
*****************************************************************/
#include "tx.hpp"

// Revolution Now
#include "errors.hpp"
#include "frame.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "macros.hpp"
#include "screen.hpp"
#include "sdl-util.hpp"

// SDL
#include "SDL.h"
#include "SDL_image.h"

// C++ standard library
#include <string>
#include <unordered_map>
#include <utility>

namespace rn {

namespace {

int g_next_texture_id{ 1 };

// Holds the number of Texture objects that hold live textures.
// Monitoring this helps to track texture leaks.
int g_live_texture_count{ 0 };

auto g_pixel_format = ::SDL_PIXELFORMAT_RGBA8888;

int g_current_render_target{ -1 };

::SDL_Texture* to_SDL_Texture( void* p ) {
  return static_cast<::SDL_Texture*>( p );
}

::SDL_Surface* to_SDL_Surface( void* p ) {
  return static_cast<::SDL_Surface*>( p );
}

::SDL_RendererFlip to_SDL( e_flip flip ) {
  switch( flip ) {
    case e_flip::none: return ::SDL_FLIP_NONE;
    case e_flip::horizontal: return ::SDL_FLIP_HORIZONTAL;
    case e_flip::vertical: return ::SDL_FLIP_VERTICAL;
  }
  UNREACHABLE_LOCATION;
}

::SDL_BlendMode to_SDL( e_tx_blend_mode mode ) {
  switch( mode ) {
    case e_tx_blend_mode::none: return SDL_BLENDMODE_NONE;
    case e_tx_blend_mode::blend: return SDL_BLENDMODE_BLEND;
    case e_tx_blend_mode::add: return SDL_BLENDMODE_ADD;
    case e_tx_blend_mode::mod: return SDL_BLENDMODE_MOD;
  }
  UNREACHABLE_LOCATION;
}

} // namespace

/****************************************************************
** Surface
*****************************************************************/
Surface::Surface( void* p ) : sf_{ p } { CHECK( sf_ ); }

Surface::Surface( Surface&& sf ) noexcept {
  DCHECK( sf.sf_, "attempt to move from a moved-from Surface." );
  sf_ = std::exchange( sf.sf_, nullptr );
}

Surface& Surface::operator=( Surface&& rhs ) noexcept {
  Surface moved( std::move( rhs ) );
  moved.swap( *this );
  return *this;
}

Surface::~Surface() { free(); }

void Surface::free() {
  if( sf_ ) {
    ::SDL_FreeSurface( to_SDL_Surface( sf_ ) );
    sf_ = nullptr;
  }
}

Delta Surface::size() const {
  auto* s = to_SDL_Surface( sf_ );
  return Delta{ W{ s->w }, H{ s->h } };
}

void Surface::optimize() {
  auto* fmt = ::SDL_AllocFormat( g_pixel_format );
  CHECK( fmt != nullptr );
  *this = Surface(
      ::SDL_ConvertSurface( to_SDL_Surface( sf_ ), fmt, 0 ) );
  ::SDL_FreeFormat( fmt );
}

Surface Surface::load_image( const char* file ) {
  ::SDL_Surface* surface = ::IMG_Load( file );
  CHECK( surface, "failed to load image: {}", file );
  return Surface( surface );
}

bool Surface::save_png( fs::path const& file ) {
  bool status = ::IMG_SavePNG( to_SDL_Surface( sf_ ),
                               file.string().c_str() ) == 0;
  if( !status ) lg.error( "failed to save png file {}.", file );
  return status;
}

Surface Surface::create( Delta delta ) {
  auto* fmt = ::SDL_AllocFormat( g_pixel_format );
  CHECK( fmt != nullptr );
  auto res = Surface( ::SDL_CreateRGBSurface(
      0, delta.w._, delta.h._, fmt->BitsPerPixel, 0, 0, 0, 0 ) );
  ::SDL_FreeFormat( fmt );
  res.optimize();
  return res;
}

Surface Surface::scaled( Delta target_size ) {
  auto target_surface = Surface::create( target_size );
  CHECK( !::SDL_BlitScaled( to_SDL_Surface( sf_ ), NULL,
                            to_SDL_Surface( target_surface.sf_ ),
                            NULL ) );
  return target_surface;
}

void Surface::lock() {
  ::SDL_LockSurface( to_SDL_Surface( sf_ ) );
}

void Surface::unlock() {
  ::SDL_UnlockSurface( to_SDL_Surface( sf_ ) );
}

/****************************************************************
** Texture
*****************************************************************/
Texture::Texture( void* tx )
  : tx_( tx ), id_{ g_next_texture_id++ } {
  CHECK( tx_ );
  g_live_texture_count++;
}

Texture::Texture( Texture&& tx ) noexcept {
  // !! here we don't check for tx.tx_ == nullptr because that is
  // a valid Texture, namely, the default texture (screen). Al-
  // though this may be SDL specific.
  tx_ = std::exchange( tx.tx_, nullptr );
  id_ = std::exchange( tx.id_, 0 );
}

Texture& Texture::operator=( Texture&& rhs ) noexcept {
  Texture moved( std::move( rhs ) );
  moved.swap( *this );
  return *this;
}

bool Texture::operator==( Texture const& rhs ) const {
  if( this == &rhs ) return true;
  // Because textures have move semantics they should never be
  // equal unless they both correspond to the screen.
  if( id_ == 0 && rhs.id_ == 0 ) {
    DCHECK( tx_ == nullptr );
    DCHECK( rhs.tx_ == nullptr );
    return true;
  }
  // At least one texture is not "the screen", so we will return
  // false, but before we do, as a sanity check:
  DCHECK( id_ != rhs.id_ );
  DCHECK( tx_ != rhs.tx_ );
  return false;
}

void Texture::free() {
  if( tx_ != nullptr ) {
    g_live_texture_count--;
    ::SDL_DestroyTexture( to_SDL_Texture( tx_ ) );
  }
  tx_ = nullptr;
  id_ = 0;
}

Texture::~Texture() { free(); }

Texture Texture::from_surface( Surface const& surface ) {
  // auto* optimized =
  //    optimize_surface( surface, [>release_input=<]false );
  ASSIGN_CHECK(
      texture, ::SDL_CreateTextureFromSurface(
                   g_renderer, to_SDL_Surface( surface.sf_ ) ) );
  return Texture( texture );
}

Texture& Texture::screen() {
  static Texture tx{};
  return tx;
}

Texture Texture::create( Delta size ) {
  bool is_target = true;

  ::SDL_TextureAccess access = is_target
                                   ? ::SDL_TEXTUREACCESS_TARGET
                                   : ::SDL_TEXTUREACCESS_STATIC;
  return Texture( ::SDL_CreateTexture(
      g_renderer, g_pixel_format, access, size.w._, size.h._ ) );
}

Texture Texture::load_image( const char* file ) {
  return Texture::from_surface( Surface::load_image( file ) );
}

Texture Texture::load_image( fs::path const& path ) {
  return load_image( path.string().c_str() );
}

Delta Texture::size() const {
  if( !tx_ ) return main_window_logical_size();
  int w, h;
  ::SDL_QueryTexture( to_SDL_Texture( tx_ ), nullptr, nullptr,
                      &w, &h );
  return { W( w ), H( h ) };
}

void Texture::set_render_target() const {
  if( g_current_render_target == id() ) return;
  CHECK( !::SDL_SetRenderTarget( g_renderer,
                                 to_SDL_Texture( tx_ ) ) );
  g_current_render_target = id();
  event_counts()["set-render-target"].tick();
}

Surface Texture::to_surface( Opt<Delta> override_size ) const {
  Delta delta       = override_size.value_or( size() );
  auto  surface     = Surface::create( delta );
  auto* sdl_surface = to_SDL_Surface( surface.sf_ );
  set_render_target();
  CHECK( ::SDL_RenderReadPixels(
             g_renderer, NULL, g_pixel_format,
             sdl_surface->pixels, sdl_surface->pitch ) == 0 );
  return surface;
}

bool Texture::save_png( fs::path const& file ) {
  lg.info( "writing png file {}.", file );
  return to_surface().save_png( file );
}

void Texture::set_blend_mode( e_tx_blend_mode mode ) {
  // For some reason SDL complains when we try to set the blend
  // mode for the default texture.
  if( is_screen() ) return;
  // At some point try checking the error here.
  ::SDL_SetTextureBlendMode( to_SDL_Texture( tx_ ),
                             to_SDL( mode ) );
}

void Texture::set_alpha_mod( uint8_t alpha ) {
  CHECK( !::SDL_SetTextureAlphaMod( to_SDL_Texture( tx_ ),
                                    alpha ) );
}

Matrix<Color> Texture::pixels() const {
  auto surface = to_surface();

  auto* fmt = ::SDL_AllocFormat( g_pixel_format );
  CHECK( fmt );

  // This is the only one we use in the game for rendering.
  CHECK( fmt->BitsPerPixel == 32 );

  auto delta = size();
  lg.trace( "reading texture pixel data of size {}", delta );

  Matrix<Color> res( delta );
  surface.lock();

  auto rect = Rect::from( Coord{}, delta );
  for( auto coord : rect ) {
    ASSIGN_CHECK_OPT( idx, rect.rasterize( coord ) );
    Uint32 pixel =
        ( (Uint32*)to_SDL_Surface( surface.sf_ )->pixels )[idx];
    res[coord] = from_SDL( color_from_pixel( fmt, pixel ) );
  }

  surface.unlock();
  ::SDL_FreeFormat( fmt );

  return res;
}

void Texture::copy_to( Texture& to, Rect const& src,
                       Rect const& dst, double angle,
                       e_flip flip ) const {
  to.set_render_target();
  auto sdl_src = to_SDL( src );
  auto sdl_dst = to_SDL( dst );
  CHECK( !::SDL_RenderCopyEx( g_renderer, to_SDL_Texture( tx_ ),
                              &sdl_src, &sdl_dst, angle, nullptr,
                              to_SDL( flip ) ) );
}

// TODO: try rewriting this implementationn by calling the above
// copy_to.
void Texture::copy_to( Texture& to, Opt<Rect> src,
                       Opt<Rect> dest ) const {
  ::SDL_Rect  sdl_src{};
  ::SDL_Rect  sdl_dst{};
  ::SDL_Rect* p_sdl_src = nullptr;
  ::SDL_Rect* p_sdl_dst = nullptr;
  if( src ) {
    sdl_src   = to_SDL( *src );
    p_sdl_src = &sdl_src;
  }
  if( dest ) {
    sdl_dst   = to_SDL( *dest );
    p_sdl_dst = &sdl_dst;
  }
  to.set_render_target();
  CHECK( !::SDL_RenderCopy( g_renderer, to_SDL_Texture( tx_ ),
                            p_sdl_src, p_sdl_dst ) );
}

void Texture::copy_to( Texture& to ) const {
  to.set_render_target();
  CHECK( !::SDL_RenderCopy( g_renderer, to_SDL_Texture( tx_ ),
                            nullptr, nullptr ) );
}

void Texture::fill( Color const& color ) {
  set_render_target();
  set_blend_mode( e_tx_blend_mode::none );
  ::SDL_SetRenderDrawColor( g_renderer, color.r, color.g,
                            color.b, color.a );
  ::SDL_RenderClear( g_renderer );
  // TODO: this shouldn't be necessary since anyone who is re-
  // lying on blend mode should be setting it prior to doing any
  // operations.  Try removing it.
  set_blend_mode( e_tx_blend_mode::blend );
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

/****************************************************************
** Debugging
*****************************************************************/
int live_texture_count() { return g_live_texture_count; }

} // namespace rn
