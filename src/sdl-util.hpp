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

// base-util
#include "base-util/non-copyable.hpp"

// SDL
#include "SDL.h"
#include "SDL_image.h"

// c++ standard library
#include <optional>
#include <ostream>

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

void render_texture( Texture const& texture, SDL_Rect source,
                     SDL_Rect dest, double angle,
                     SDL_RendererFlip flip );

ND bool is_window_fullscreen();
void    set_fullscreen( bool fullscreen );
void    toggle_fullscreen();

ND ::SDL_Rect to_SDL( Rect const& rect );

// Retrive the width and height of a texture in a Rect.
ND Delta texture_delta( Texture const& tx );

// Copies one texture to another at the destination point without
// scaling. Destination texture can be nullopt for default
// rendering target. Returns true on success, false otherwise.
ND bool copy_texture( Texture const& from, OptCRef<Texture> to,
                      Y y, X x );
ND bool copy_texture( Texture const& from, OptCRef<Texture> to,
                      Coord const& coord );

ND Texture create_texture( W w, H h );

::SDL_Surface* create_surface( Delta delta );

Delta screen_logical_size();
// Same but with origin at 0,0
Rect screen_logical_rect();

void save_texture_png( Texture const& tx, fs::path const& file );
void grab_screen( fs::path const& file );

void clear_texture_black( Texture const& tx );

::SDL_Color to_SDL( Color color );
Color       from_SDL( ::SDL_Color color );

void set_render_draw_color( Color color );
void render_rect( OptCRef<Texture> tx, Color color,
                  Rect const& rect );
void render_fill_rect( OptCRef<Texture> tx, Color color,
                       Rect const& rect );

// Mainly for testing.  Just waits for the user to press 'q'.
void wait_for_q();

} // namespace rn

std::ostream& operator<<( std::ostream&     out,
                          ::SDL_Rect const& r );
