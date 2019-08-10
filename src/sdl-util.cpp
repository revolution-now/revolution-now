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
#include "aliases.hpp"
#include "errors.hpp"
#include "fmt-helper.hpp"
#include "frame.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "macros.hpp"
#include "screen.hpp"
#include "util.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/string.hpp"

// Range-v3
#include "range/v3/view/group_by.hpp"
#include "range/v3/view/transform.hpp"

// Abseil
#include "absl/strings/str_replace.h"

// C++ standard library
#include <cmath>
#include <iomanip>
#include <unordered_map>
#include <vector>

using namespace std;

namespace rn {

namespace {

vector<Rect> clip_stack;

} // namespace

void check_compile_link_version(
    string_view module_name, ::SDL_version const* link_version,
    ::SDL_version const& compiled_version ) {
  lg.debug( "SDL {}: compiled with version: {}.{}.{}",
            module_name, compiled_version.major,
            compiled_version.minor, compiled_version.patch );
  lg.debug( "SDL {}:  running with version: {}.{}.{}",
            module_name, link_version->major,
            link_version->minor, link_version->patch );
  CHECK( compiled_version.major == link_version->major,
         "This game was compiled with a version of SDL {} whose "
         "major version number ({}) is different from the major "
         "version number of the runtime library ({})",
         module_name, compiled_version.major,
         link_version->major );

  if( compiled_version.minor != link_version->minor ) {
    lg.warn(
        "This game was compiled with a version of SDL {} whose "
        "minor version number ({}) is different from the minor "
        "version number of the runtime library ({})",
        module_name, compiled_version.minor,
        link_version->minor );
  }
}

MachinePowerInfo machine_power_info() {
  MachinePowerInfo info;

  int  seconds_left, battery_percent;
  auto status =
      ::SDL_GetPowerInfo( &seconds_left, &battery_percent );
  switch( status ) {
    // Not plugged in, running on the battery.
    case SDL_POWERSTATE_ON_BATTERY:
      info.power_state = e_power_state::on_battery;
      break;
    // Plugged in, no battery available.
    case SDL_POWERSTATE_NO_BATTERY:
      info.power_state = e_power_state::plugged_no_battery;
      break;
    // Plugged in, charging battery.
    case SDL_POWERSTATE_CHARGING:
      info.power_state = e_power_state::plugged_charging;
      break;
    // Plugged in, battery charged.
    case SDL_POWERSTATE_CHARGED:
      info.power_state = e_power_state::plugged_charged;
      break;
    // Cannot determine power status.
    case SDL_POWERSTATE_UNKNOWN:
    default: info.power_state = e_power_state::unknown; break;
  }
  if( battery_percent != -1 )
    info.battery_percentage = battery_percent;
  return info;
}

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

  auto power_info = machine_power_info();
  lg.info( "power info: {}, Battery: {}%",
           power_info.power_state,
           power_info.battery_percentage );
}

void cleanup_sdl() { ::SDL_Quit(); }

REGISTER_INIT_ROUTINE( sdl );

Texture from_SDL( ::SDL_Texture* tx ) { return Texture( tx ); }

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
  from.copy_to( to, /*src=*/nullopt, /*dest=*/rect );
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
  from.copy_to( to, /*src=*/nullopt, /*dest=*/rect );
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
  fill_texture( tx, color );
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
  auto black = create_texture( tx.size(), Color{0, 0, 0, 255} );

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
      create_texture( tx.size(), Color{255, 255, 255, 255} );
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
          {{" ", "-"}} ) );
}

void clear_texture_black( Texture& tx ) {
  tx.set_render_target();
  ::SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, 255 );
  ::SDL_RenderClear( g_renderer );
}

void clear_texture( Texture& tx, Color color ) {
  tx.set_render_target();
  ::SDL_SetRenderDrawColor( g_renderer, color.r, color.g,
                            color.b, 255 );
  ::SDL_RenderClear( g_renderer );
}

void clear_texture_transparent( Texture& tx ) {
  tx.set_render_target();
  tx.set_blend_mode( e_tx_blend_mode::none );
  ::SDL_SetRenderDrawColor( g_renderer, 0, 0, 0, 0 );
  ::SDL_RenderClear( g_renderer );
  // TODO: this shouldn't be necessary since anyone who is
  // relying on blend mode should be setting it prior to
  // doing any operations.
  tx.set_blend_mode( e_tx_blend_mode::blend );
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

void render_fill_rect( Texture& tx, Color color,
                       Rect const& rect ) {
  tx.set_render_target();
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
  UNREACHABLE_LOCATION;
}

// WARNING: this is slow, only use in pre-rendered textures.
void render_fill_rect_rounded( Texture& tx, Color color,
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

void fill_texture( Texture& tx, Color color ) {
  render_fill_rect( tx, color,
                    Rect::from( Coord{}, tx.size() ) );
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

// Caller must set blend mode!
void render_points( Texture& tx, Color color,
                    vector<Coord> const& points ) {
  auto to_sdl = []( Coord const& coord ) {
    return to_SDL( coord );
  };
  auto sdl_points = util::map( to_sdl, points );
  tx.set_render_target();
  set_render_draw_color( color );
  CHECK( !::SDL_RenderDrawPoints( g_renderer, &sdl_points[0],
                                  sdl_points.size() ) );
}

} // namespace rn
