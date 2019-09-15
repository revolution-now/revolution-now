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
#include "config-files.hpp"
#include "coord.hpp"
#include "deferred.hpp"
#include "frame.hpp"
#include "gfx.hpp"
#include "menu.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "ttf.hpp"
#include "views.hpp"

// Revolution Now (config)
#include "../config/ucl/rn.inl"

using namespace std;

namespace rn {

namespace {

constexpr uint8_t console_alpha = 200;
constexpr uint8_t edit_alpha    = 200;
constexpr uint8_t text_alpha    = 220;
constexpr uint8_t stats_alpha   = 255;

size_t constexpr max_dbg_log_lines = 100000;
vector<string> dbg_log;

struct ConsolePlane : public Plane {
  ConsolePlane() = default;
  void initialize() override {
    // FIXME: move this into method that gets called when logical
    // window size changes.
    le_view_.emplace(
        font::standard(), main_window_logical_size().w,
        []( string const& ) {}, Color::blue(), Color::white(),
        ">" );
  }
  bool enabled() const override { return true; }
  bool covers_screen() const override { return false; }
  void draw( Texture& tx ) const override {
    clear_texture_transparent( tx );
    if( !show_ ) return;
    auto rect =
        Rect::from( Coord{}, main_window_logical_size() );
    rect.y += rect.h / 3_sy * 2_sy;
    rect.h /= 3_sy;
    rect =
        rect.shifted_by( Delta{0_w, -le_view_.get().delta().h} );
    render_fill_rect( tx, Color{0, 0, 255, console_alpha},
                      rect );

    auto text_color  = Color{255, 255, 255, text_alpha};
    auto stats_color = Color{255, 255, 255, stats_alpha};

    // auto info_start = Coord{} + 16_h;

    // Coord mouse;
    //::SDL_GetMouseState( &mouse.x._, &mouse.y._ );

    //{
    //  auto mouse_coords = fmt::format( "unscaled: {}", mouse );
    //  auto const& mouse_coords_tx = render_text(
    //      config_rn.console.font, Color::white(), mouse_coords
    //      );
    //  copy_texture( mouse_coords_tx, tx, info_start );
    //  info_start += mouse_coords_tx.size().h;
    //}

    //// mouse.clip( ... );
    // mouse.x /= g_resolution_scale_factor.sx;
    // mouse.y /= g_resolution_scale_factor.sy;
    //{
    //  auto mouse_coords = fmt::format( "  scaled: {}", mouse );
    //  auto const& mouse_coords_tx = render_text(
    //      config_rn.console.font, Color::white(), mouse_coords
    //      );
    //  copy_texture( mouse_coords_tx, tx, info_start );
    //  info_start += mouse_coords_tx.size().h;
    //}

    auto info_start = rect.lower_right();

    auto frame_rate =
        fmt::format( "fps: {:.1f}", avg_frame_rate() );
    auto const& frame_rate_tx = render_text(
        config_rn.console.font, Color::white(), frame_rate );
    copy_texture( frame_rate_tx, tx,
                  info_start - frame_rate_tx.size() );
    info_start -= frame_rate_tx.size().h;

    auto tx_count =
        fmt::format( "tx count: {}", live_texture_count() );
    // This needs to use an uncached rendering function so that
    // it doesn't cause a feedback loop that continually in-
    // creases the texture count. This is because rendering this
    // number, in general, causes a new texture to be created.
    auto tx_count_tx = ttf_render_text_line_uncached(
        config_rn.console.font, Color::white(), tx_count );
    copy_texture( tx_count_tx, tx,
                  info_start - tx_count_tx.size() );
    info_start -= tx_count_tx.size().h;

    auto text_tx_count =
        fmt::format( "text cache size: {}", text_cache_size() );
    // This needs to use an uncached rendering function so that
    // it doesn't cause a feedback loop that continually in-
    // creases the texture count. This is because rendering this
    // number, in general, causes a new texture to be created.
    auto text_tx_count_tx = ttf_render_text_line_uncached(
        config_rn.console.font, Color::white(), text_tx_count );
    copy_texture( text_tx_count_tx, tx,
                  info_start - text_tx_count_tx.size() );
    info_start -= text_tx_count_tx.size().h;

    // FIXME: better way to get this?
    auto text_height = frame_rate_tx.size().h;

    auto        dashes = fmt::format( "--------------------" );
    auto const& dashes_tx = render_text( config_rn.console.font,
                                         text_color, dashes );
    copy_texture( dashes_tx, tx, info_start - dashes_tx.size() );
    info_start -= dashes_tx.size().h;

    for( auto const& [name, mv_avg] : event_counts() ) {
      auto formatted = fmt::format(
          "{}/f: {}", name,
          std::lround( mv_avg.average() / avg_frame_rate() ) );
      auto const& src_tx = render_text( config_rn.console.font,
                                        stats_color, formatted );
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
      auto const& src_tx = render_text( config_rn.console.font,
                                        text_color, dbg_log[i] );
      copy_texture( src_tx, tx, log_px_start );
      log_px_start += src_tx.size().h;
    }

    auto edit_rect =
        Rect::from( main_window_logical_rect().lower_left() -
                        le_view_.get().delta().h,
                    le_view_.get().delta() );

    render_rect( tx, Color::white().with_alpha( edit_alpha ),
                 rect );
    le_view_.get().draw( tx, edit_rect.upper_left() );
  }

  bool input( input::event_t const& event ) override {
    if( !util::holds<input::key_event_t>( event ) ) return false;
    auto const& key_event =
        *std::get_if<input::key_event_t>( &event );
    if( key_event.change == input::e_key_change::down &&
        key_event.keycode == ::SDLK_SLASH ) {
      show_ = !show_;
      return true;
    }
    if( !show_ ) return false;
    return le_view_.get().on_key( key_event );
  }

  bool                         show_{false};
  deferred<ui::LineEditorView> le_view_{};
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
    [] { g_console_plane.show_ = !g_console_plane.show_; },
    [] { return true; } )

} // namespace rn
