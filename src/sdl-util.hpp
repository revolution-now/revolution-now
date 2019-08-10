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
#include "coord.hpp"
#include "enum.hpp"
#include "tx.hpp" // TODO: do we need this?
#include "util.hpp"

// SDL
#include "SDL.h"       // FIXME: get rid of this.
#include "SDL_image.h" // FIXME: get rid of this.

// C++ standard library
#include <optional>
#include <string_view>

namespace rn {

struct Pixel {
  Coord coord;
  Color color;
};

enum class e_( power_state,        //
               unknown,            //
               on_battery,         //
               plugged_no_battery, //
               plugged_charging,   //
               plugged_charged     //
);

struct MachinePowerInfo {
  e_power_state power_state;
  // If known, this will be populated; [0, 100].
  Opt<int> battery_percentage{};
};

// This will return as much info about the power situation as the
// system can give us.
MachinePowerInfo machine_power_info();

// Will throw an error is the game is being run with a different
// major version of the given SDL library (e.g., mixer, ttf,
// etc.) than it was built with. A warning will be issued if the
// minor version is different.
void check_compile_link_version(
    std::string_view     module_name,
    ::SDL_version const* link_version,
    ::SDL_version const& compiled_version );

// Make an RAII version of this
void push_clip_rect( Rect const& rect );
void pop_clip_rect();

::SDL_Rect to_SDL( Rect const& rect );
Rect       from_SDL( ::SDL_Rect const& rect );

ND ::SDL_Point to_SDL( Coord const& coord );

// This one is used for individual sprites that may need to be
// flipped or rotated.
void copy_texture( Texture const& from, Texture& to,
                   Rect const& src, Rect const& dst,
                   double angle, e_flip flip );

// Copies one texture to another at the destination point without
// scaling. Destination texture can be nullopt for default
// rendering target.
void copy_texture( Texture const& from, Texture& to,
                   Coord const& dst_coord );
void copy_texture( Texture const& from, Texture& to,
                   Rect const& src, Coord dst_coord );
// This one may stretch/scale if the rects are not the same size.
void copy_texture( Texture const& from, Texture& to,
                   Rect const& src, Rect const& dst );
// With alpha.  Note, that this does not seem to behave the same
// as a "regular" copy_texture call when setting alpha == 255,
// so we should only use this when we need to specify the alpha.
void copy_texture_alpha( Texture& from, Texture& to,
                         Coord const& dst_coord, uint8_t alpha );
// Same as above but destination coord is (0,0). Note this should
// not be used for rendering to the main texture since we don't
// start at (0,0) there; see `copy_texture_to_screen` below.
void copy_texture( Texture const& from, Texture& to );
// This one is used to copy a (logical-screen-sized) texture to
// the main texture for display. The destination origin will be
// the drawing origin and there will be no stretching.
void copy_texture_to_main( Texture const& from );
// Copies the texture potentially with stretching (which is
// implicit in the ratios of the sizes of the rects).
void copy_texture_stretch( Texture const& from, Texture& to,
                           Rect const& src, Rect const& dest );

// Clones size and content, including alpha.
Texture clone_texture( Texture const& tx );

ND Texture create_texture( Delta delta );
ND Texture create_texture( Delta delta, Color const& color );
ND Texture create_texture_transparent( Delta delta );
ND Texture create_screen_physical_sized_texture();

// This will create a new texture that is the same size as the
// given one, except that all of the pixels in the source texture
// will be converted to black (RGB={0,0,0}), but with the same
// alpha value as the source. Hence it creates a "shadow" of the
// source texture's opaque (or semi-opaque) parts.
Texture create_shadow_texture( Texture const& tx );

// Returns true on success.
// TODO: needs to be retested.
bool screenshot( fs::path const& file );
// Generate a filename in the user's home folder containing the
// current timestamp and save a screenshot there. Returns true on
// success.
bool screenshot();

void clear_texture_black( Texture& tx );
void clear_texture_transparent( Texture& tx );
void clear_texture( Texture& tx, Color color );

void fill_texture( Texture& tx, Color color );

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
void render_line( Texture& tx, Color color, Coord start,
                  Delta delta );
void render_rect( Texture& tx, Color color, Rect const& rect );
void render_fill_rect( Texture& tx, Color color,
                       Rect const& rect );

enum class rounded_corner_type { radius_2, radius_3, radius_4 };

// WARNING: this is slow, only use in pre-rendered textures.
void render_fill_rect_rounded( Texture& tx, Color color,
                               Rect const&         rect,
                               rounded_corner_type type );

// NOTE: This function does not set the blend mode, so the caller
// must set it!
//
// WARNING: this is probably slow.
void render_points( Texture& tx, Color color,
                    std::vector<Coord> const& points );

} // namespace rn
