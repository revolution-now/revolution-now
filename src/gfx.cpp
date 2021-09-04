/****************************************************************
**gfx.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-09.
*
* Description: Graphics routines.
*
*****************************************************************/
#include "gfx.hpp"

// Revolution Now
#include "logger.hpp"
#include "screen.hpp"
#include "sdl-util.hpp"
#include "time.hpp"
#include "util.hpp"

// Abseil
#include "absl/strings/str_replace.h"

namespace rn {

using namespace std;

namespace {} // namespace

void copy_texture( Texture const& from, Texture& to,
                   Rect const& src, Rect const& dst,
                   double angle, e_flip flip ) {
  from.copy_to( to, src, dst, angle, flip );
}

void copy_texture( Texture const& from, Texture& to,
                   Rect const& src, Coord dst_coord ) {
  copy_texture( from, to, src,
                src.with_new_upper_left( dst_coord ), 0, {} );
}

void copy_texture( Texture const& from, Texture& to,
                   Rect const& src, Rect const& dst ) {
  copy_texture( from, to, src, dst, 0, e_flip::none );
}

// With alpha. TODO: figure out why this doesn't behave like a
// standard copy_texture when alpha == 255.
void copy_texture_alpha( Texture& from, Texture& to,
                         Coord const& dst_coord,
                         uint8_t      alpha ) {
  CHECK( !from.is_screen() );
  // TODO: Do we really need this? If we can get rid of it then
  // the `from` parameter can be const like it probably should
  // be, and then many variables in the vicinity of the call
  // sites of this function can be made const.
  from.set_blend_mode( e_tx_blend_mode::blend );
  to.set_blend_mode( e_tx_blend_mode::blend );
  from.set_alpha_mod( alpha );
  auto rect = Rect::from( dst_coord, from.size() );
  from.copy_to( to, /*src=*/nothing, /*dest=*/rect );
  // Restore texture's alpha because that is what most actions
  // will need it to be, and we don't set it before every texture
  // copying action in this module.
  from.set_alpha_mod( 255 );
}

void copy_texture( Texture const& from, Texture& to,
                   Coord const& dst_coord ) {
  // TODO: do we actually need this?
  // from.set_blend_mode( e_tx_blend_mode::blend );
  to.set_blend_mode( e_tx_blend_mode::blend );
  auto rect = Rect::from( dst_coord, from.size() );
  from.copy_to( to, /*src=*/nothing, /*dest=*/rect );
}

void copy_texture_to_main( Texture const& from ) {
  copy_texture( from, Texture::screen(), Coord{} );
}

void copy_texture( Texture const& from, Texture& to ) {
  copy_texture( from, to, Coord{} );
}

void copy_texture_stretch( Texture const& from, Texture& to,
                           Rect const& src, Rect const& dest ) {
  // TODO: do we actually need this?
  // from.set_blend_mode( e_tx_blend_mode::blend );
  to.set_blend_mode( e_tx_blend_mode::blend );
  from.copy_to( to, /*src=*/src, /*dest=*/dest );
}

Texture clone_texture( Texture const& tx ) {
  auto res = create_texture_transparent( tx.size() );
  copy_texture( tx, res );
  return res;
}

ND Texture create_texture( Delta delta ) {
  auto tx = Texture::create( delta );
  clear_texture_black( tx );
  return tx;
}

ND Texture create_texture( Delta delta, Color const& color ) {
  auto tx = create_texture( delta );
  tx.fill( color );
  return tx;
}

ND Texture create_texture_transparent( Delta delta ) {
  auto tx = create_texture( delta );
  clear_texture_transparent( tx );
  return tx;
}

ND Texture create_screen_physical_sized_texture() {
  auto res = create_texture( whole_screen_physical_size() );
  lg.debug( "created screen-sized texture occupying {}MB.",
            res.mem_usage_mb() );
  return res;
}

Texture create_shadow_texture( Texture const& tx ) {
  auto cloned = clone_texture( tx );
  // black.a should not be relevant here.
  auto black =
      create_texture( tx.size(), Color{ 0, 0, 0, 255 } );

  // The process will be done in two stages; note that each stage
  // respects alpha gradations when performing its action, so
  // that the resulting texture will maintain the same alpha
  // pattern as the input texture.

  // Stage one: turn the cloned texture all white in its opaque
  // parts.
  //   dstRGB = (srcRGB * srcA) + dstRGB
  //   dstA = dstA
  Texture::screen().set_blend_mode( e_tx_blend_mode::add );
  cloned.set_blend_mode( e_tx_blend_mode::add );
  auto white =
      create_texture( tx.size(), Color{ 255, 255, 255, 255 } );
  white.set_blend_mode( e_tx_blend_mode::add );
  white.copy_to( cloned );

  // Stage two: turn the white parts of the cloned texture to
  // black.
  //   dstRGB = srcRGB * dstRGB
  //   dstA = dstA
  cloned.set_blend_mode( e_tx_blend_mode::mod );
  black.set_blend_mode( e_tx_blend_mode::mod );
  black.copy_to( cloned );

  // Return blend mode to a standard value just for good measure.
  cloned.set_blend_mode( e_tx_blend_mode::blend );

  return cloned;
}

void set_render_draw_color( Color color ) {
  CHECK( !::SDL_SetRenderDrawColor( g_renderer, color.r, color.g,
                                    color.b, color.a ) );
}

void clear_texture_black( Texture& tx ) {
  tx.fill( Color::black() );
}

void clear_texture_transparent( Texture& tx ) {
  tx.fill( { 0, 0, 0, 0 } );
}

void render_fill_rect( Texture& tx, Color color,
                       Rect const& rect ) {
  tx.set_render_target();
  set_render_draw_color( color );
  auto sdl_rect = to_SDL( rect );
  ::SDL_RenderFillRect( g_renderer, &sdl_rect );
}

void render_line( Texture& tx, Color color, Coord start,
                  Delta delta ) {
  // The SDL rendering method used below includes end points, so
  // we must avoid calling it if the line will have zero length.
  if( delta == Delta::zero() ) return;
  tx.set_render_target();
  set_render_draw_color( color );
  Coord end = start + delta.trimmed_by_one();
  ::SDL_RenderDrawLine( g_renderer, start.x._, start.y._,
                        end.x._, end.y._ );
}

void render_rect( Texture& tx, Color color, Rect const& rect ) {
  tx.set_render_target();
  set_render_draw_color( color );
  auto sdl_rect = to_SDL( rect );
  ::SDL_RenderDrawRect( g_renderer, &sdl_rect );
}

bool screenshot( fs::path const& file ) {
  auto logical_screen =
      Texture::screen()
          .to_surface( main_window_physical_size() )
          .scaled( main_window_logical_size() );
  auto logical_tx = Texture::from_surface( logical_screen );
  // Unfortunately the above tx will not support being a render
  // target, so we need to create yet another texture. It needs
  // to be a render target because we're going to need to set it
  // as such to read from it in the save_png function.
  auto logical_tx_target = Texture::create( logical_tx.size() );
  copy_texture( logical_tx, logical_tx_target );
  lg.info( "saving screenshot with size {}x{} to \"{}\".",
           logical_tx_target.size().w._,
           logical_tx_target.size().h._, file.string() );
  return logical_tx_target.save_png( file );
}

bool screenshot() {
  auto home_folder = user_home_folder();
  if( !home_folder ) {
    lg.error(
        "Cannot save screenshot; HOME variable not set in "
        "environment." );
    return false;
  }
  return screenshot(
      ( *home_folder ) /
      absl::StrReplaceAll(
          fmt::format( "revolution-now-screenshot-{}.png",
                       util::to_string( Clock_t::now() ) ),
          { { " ", "-" } } ) );
}

} // namespace rn
