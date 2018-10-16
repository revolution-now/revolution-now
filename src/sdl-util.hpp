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

#include "base-util.hpp"

#include "util/non-copyable.hpp"

#include <SDL.h>
#include <SDL_image.h>

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

private:
  friend Texture from_SDL( ::SDL_Texture* tx );
  explicit Texture( ::SDL_Texture* tx );
  ::SDL_Texture* tx_{nullptr};
};

constexpr uint8_t alpha_opaque{255};
constexpr uint8_t max_saturation{255};

struct Color : public ::SDL_Color {
  Color() : ::SDL_Color{0, 0, 0, alpha_opaque} {}
  Color( Uint8 r, Uint8 g, Uint8 b, Uint8 a )
    : ::SDL_Color{r, g, b, a} {}

  static Color red() {
    return {max_saturation, 0, 0, alpha_opaque};
  }
  static Color green() {
    return {0, max_saturation, 0, alpha_opaque};
  }
  static Color blue() {
    return {0, 0, max_saturation, alpha_opaque};
  }
  static Color white() {
    return {max_saturation, max_saturation, max_saturation,
            alpha_opaque};
  }
  static Color black() { return {0, 0, 0, alpha_opaque}; }
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

ND Texture& load_texture( const char* file );

void render_texture( Texture const& texture, SDL_Rect source,
                     SDL_Rect dest, double angle,
                     SDL_RendererFlip flip );

ND bool is_window_fullscreen();
void    set_fullscreen( bool fullscreen );
void    toggle_fullscreen();

ND ::SDL_Rect to_SDL( Rect const& rect );

// Retrive the width and height of a texture in a Rect. The
// Rect's x and y will be zero.
ND Rect texture_rect( Texture const& texture );

// Copies one texture to another at the destination point without
// scaling. Destination texture can be nullopt for default
// rendering target. Returns true on success, false otherwise.
ND bool copy_texture( Texture const& from, OptCRef<Texture> to,
                      Y y, X x );
ND bool copy_texture( Texture const& from, OptCRef<Texture> to,
                      Coord const& coord );

ND Texture create_texture( W w, H h );

void clear_texture_black( Texture const& tx );

void set_render_draw_color( Color const& color );
void render_rect( OptCRef<Texture> tx, Color const& color,
                  Rect const& rect );
void render_fill_rect( OptCRef<Texture> tx, Color const& color,
                       Rect const& rect );

} // namespace rn

std::ostream& operator<<( std::ostream&     out,
                          ::SDL_Rect const& r );
