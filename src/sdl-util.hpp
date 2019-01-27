/****************************************************************
**sdl-util.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Interface for calling SDL functions
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "color.hpp"
#include "geo-types.hpp"
#include "util.hpp"

// base-util
#include "base-util/non-copyable.hpp"

// SDL
#include "SDL.h"
#include "SDL_image.h"

// c++ standard library
#include <optional>

namespace rn {

// RAII wrapper for SDL_Texture.
class Texture : public util::non_copy_non_move {
public:
  Texture()                 = default;
  Texture( Texture const& ) = delete;
  Texture( Texture&& tx ) noexcept;
  Texture& operator=( Texture const& rhs ) = delete;
  Texture& operator=( Texture&& rhs ) noexcept;
  ~Texture();

  // NOLINTNEXTLINE(hicpp-explicit-conversions)
  operator ::SDL_Texture*() const { return tx_; }

  ::SDL_Texture* get() const { return tx_; }
  static Texture from_surface( ::SDL_Surface* surface );

  // For convenience.
  Delta size() const;

  void free();

private:
  friend Texture from_SDL( ::SDL_Texture* tx );
  explicit Texture( ::SDL_Texture* tx );
  ::SDL_Texture* tx_{nullptr};
};

using TextureRef = std::reference_wrapper<Texture>;

void init_game();

void init_sdl();

void create_window();

void print_video_stats();

void create_renderer();

void cleanup();

void set_render_target( OptCRef<Texture> tx );

// Make an RAII version of this
void push_clip_rect( Rect const& rect );
void pop_clip_rect();

::SDL_Surface* load_surface( const char* file );
ND Texture& load_texture( const char* file );
ND Texture& load_texture( fs::path const& path );

ND bool is_window_fullscreen();
void    set_fullscreen( bool fullscreen );
void    toggle_fullscreen();

ND ::SDL_Rect to_SDL( Rect const& rect );

ND ::SDL_Point to_SDL( Coord const& coord );

// Retrive the width and height of a texture in a Rect.
ND Delta texture_delta( Texture const& tx );

// This one is used for individual sprites that may need to be
// flipped or rotated.
void copy_texture( Texture const& from, Texture const& to,
                   Rect const& src, Rect const& dst,
                   double angle, SDL_RendererFlip flip );

// Copies one texture to another at the destination point without
// scaling. Destination texture can be nullopt for default
// rendering target.
void copy_texture( Texture const& from, OptCRef<Texture> to,
                   Coord const& dst_coord );
// Same as above but destination coord is (0,0). Note this should
// not be used for rendering to the main texture since we don't
// start at (0,0) there; see `copy_texture_to_screen` below.
void copy_texture( Texture const& from, Texture const& to );
// This one is used to copy a (logical-screen-sized) texture to
// the main texture for display. The destination origin will be
// the drawing origin and there will be no stretching.
void copy_texture_to_main( Texture const& from );
// Copies the texture potentially with stretching (which is
// implicit in the ratios of the sizes of the rects).
void copy_texture_stretch( Texture const&   from,
                           OptCRef<Texture> to, Rect const& src,
                           Rect const& dest );

ND Texture create_texture( W w, H h );
ND Texture create_texture( Delta delta );
ND Texture create_screen_sized_texture();

::SDL_Surface* create_surface( Delta delta );

// WARNING: Very slow function, should not be done in real time.
// This is because it reads data from a texture.
Matrix<Color> texture_pixels( Texture const& tx );

Delta screen_logical_size();
// Same but with origin at 0,0
Rect screen_logical_rect();

void save_texture_png( Texture const& tx, fs::path const& file );
void grab_screen( fs::path const& file );

void clear_texture_black( Texture const& tx );
void clear_texture_transparent( Texture const& tx );

::SDL_Color color_from_pixel( SDL_PixelFormat* fmt,
                              Uint32           pixel );
::SDL_Color to_SDL( Color color );
Color       from_SDL( ::SDL_Color color );

void set_render_draw_color( Color color );
// This takes a delta so that it is obvious how long the line
// will be. The SDL api method used includes end points, so if we
// took a Coord as the end point then we'd have to subtract one
// from each length in order to allow for zero-length lines with
// that approach.
void render_line( Texture const& tx, Color color, Coord start,
                  Delta delta );
void render_rect( OptCRef<Texture> tx, Color color,
                  Rect const& rect );
void render_fill_rect( OptCRef<Texture> tx, Color color,
                       Rect const& rect );
void render_fill_rect( Texture const& tx, Color color );

} // namespace rn
