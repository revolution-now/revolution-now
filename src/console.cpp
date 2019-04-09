/****************************************************************
**console.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-02-03.
*
* Description: The developer/mod console.
*
*****************************************************************/
#include "console.hpp"

// Revolution Now
#include "coord.hpp"
#include "fonts.hpp"
#include "frame.hpp"
#include "menu.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "sdl-util.hpp"

using namespace std;

namespace rn {

namespace {

uint8_t console_alpha = 128;

size_t constexpr max_dbg_log_lines = 100000;
vector<string> dbg_log;

struct ConsolePlane : public Plane {
  ConsolePlane() = default;
  bool enabled() const override { return enabled_; }
  bool covers_screen() const override { return false; }
  void draw( Texture const& tx ) const override {
    clear_texture_transparent( tx );
    auto rect =
        Rect::from( Coord{}, main_window_logical_size() );
    rect.y += rect.h / 3_sy * 2_sy;
    rect.h /= 3_sy;
    render_fill_rect( tx, Color{0, 0, 255, console_alpha},
                      rect );

    auto text_color = Color{255, 255, 255, console_alpha};

    // auto info_start = Coord{} + 16_h;

    // Coord mouse;
    //::SDL_GetMouseState( &mouse.x._, &mouse.y._ );

    //{
    //  auto mouse_coords = fmt::format( "unscaled: {}", mouse );
    //  auto mouse_coords_tx = render_text_line_solid(
    //      fonts::standard, Color::white(), mouse_coords );
    //  copy_texture( mouse_coords_tx, tx, info_start );
    //  info_start += mouse_coords_tx.size().h;
    //}

    //// mouse.clip( ... );
    // mouse.x /= g_resolution_scale_factor.sx;
    // mouse.y /= g_resolution_scale_factor.sy;
    //{
    //  auto mouse_coords = fmt::format( "  scaled: {}", mouse );
    //  auto mouse_coords_tx = render_text_line_solid(
    //      fonts::standard, Color::white(), mouse_coords );
    //  copy_texture( mouse_coords_tx, tx, info_start );
    //  info_start += mouse_coords_tx.size().h;
    //}

    auto info_start = rect.lower_right();

    auto frame_rate =
        fmt::format( "fps: {:.1f}", avg_frame_rate() );
    auto frame_rate_tx = render_text_line_solid(
        fonts::standard, Color::white(), frame_rate );
    copy_texture( frame_rate_tx, tx,
                  info_start - frame_rate_tx.size() );
    info_start -= frame_rate_tx.size().h;

    // FIXME: better way to get this?
    auto text_height = frame_rate_tx.size().h;

    auto dashes    = fmt::format( "--------------------" );
    auto dashes_tx = render_text_line_solid(
        fonts::standard, text_color, dashes );
    copy_texture( dashes_tx, tx, info_start - dashes_tx.size() );
    info_start -= dashes_tx.size().h;

    for( auto const& [name, mv_avg] : event_counts() ) {
      auto formatted = fmt::format(
          "{}/f: {}", name,
          std::lround( mv_avg.average() / avg_frame_rate() ) );
      auto src_tx = render_text_line_solid(
          fonts::standard, text_color, formatted );
      copy_texture( src_tx, tx, info_start - src_tx.size() );
      info_start -= src_tx.size().h;
    }

    size_t max_lines = rect.h / text_height;
    // Render the log
    size_t log_start = dbg_log.size() < max_lines
                           ? 0
                           : dbg_log.size() - max_lines;
    auto log_px_start = rect.upper_left();
    for( auto i = log_start; i < dbg_log.size(); ++i ) {
      CHECK( i < dbg_log.size() );
      auto src_tx = render_text_line_solid(
          fonts::standard, text_color, dbg_log[i] );
      copy_texture( src_tx, tx, log_px_start );
      log_px_start += src_tx.size().h;
    }
  }

  bool enabled_{false};
};

ConsolePlane g_console_plane;

void trim_dbg_log() {
  if( dbg_log.size() > max_dbg_log_lines )
    dbg_log = vector<string>(
        dbg_log.begin() + max_dbg_log_lines / 2, dbg_log.end() );
}

} // namespace

void log_to_debug_console( std::string const& msg ) {
  dbg_log.push_back( msg );
  trim_dbg_log();
}

void log_to_debug_console( std::string&& msg ) {
  dbg_log.emplace_back( msg );
  trim_dbg_log();
}

Plane* console_plane() { return &g_console_plane; }

//
MENU_ITEM_HANDLER(
    e_menu_item::toggle_console,
    [] { g_console_plane.enabled_ = !g_console_plane.enabled_; },
    [] { return true; } )

} // namespace rn
