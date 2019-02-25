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
#include "fonts.hpp"
#include "frame.hpp"
#include "geo-types.hpp"
#include "menu.hpp"
#include "plane.hpp"
#include "sdl-util.hpp"

using namespace std;

namespace rn {

namespace {

struct ConsolePlane : public Plane {
  ConsolePlane() = default;
  bool enabled() const override { return enabled_; }
  bool covers_screen() const override { return false; }
  void draw( Texture const& tx ) const override {
    clear_texture_transparent( tx );
    auto rect = Rect::from( Coord{}, tx.size() );
    rect.y += rect.h / 3_sy * 2_sy;
    rect.h /= 3_sy;
    render_fill_rect( tx, Color{0, 0, 255, 64}, rect );

    auto info_start = rect.lower_right();

    auto frame_rate =
        fmt::format( "fps: {:.1f}", avg_frame_rate() );
    auto frame_rate_tx = render_text_line_fast(
        fonts::standard, Color::white(), frame_rate );
    copy_texture( frame_rate_tx, tx,
                  info_start - frame_rate_tx.size() );
    info_start -= frame_rate_tx.size().h;

    auto srt_rate = fmt::format(
        "srt/f: {:.1f}", double( total_set_render_target() ) /
                             total_frame_count() );
    auto srt_rate_tx = render_text_line_fast(
        fonts::standard, Color::white(), srt_rate );
    copy_texture( srt_rate_tx, tx,
                  info_start - srt_rate_tx.size() );
    info_start -= srt_rate_tx.size().h;

    auto dashes    = fmt::format( "--------------------" );
    auto dashes_tx = render_text_line_fast(
        fonts::standard, Color::white(), dashes );
    copy_texture( dashes_tx, tx, info_start - dashes_tx.size() );
    info_start -= dashes_tx.size().h;

    for( auto const& [name, mv_avg] : event_counts() ) {
      auto formatted = fmt::format(
          "{}/f: {}", name,
          std::lround( mv_avg.average() / avg_frame_rate() ) );
      auto src_tx = render_text_line_fast(
          fonts::standard, Color::white(), formatted );
      copy_texture( src_tx, tx, info_start - src_tx.size() );
      info_start -= src_tx.size().h;
    }
  }

  bool enabled_{true};
};

ConsolePlane g_console_plane;

} // namespace

Plane* console_plane() { return &g_console_plane; }

MENU_ITEM_HANDLER(
    e_menu_item::toggle_console,
    [] { g_console_plane.enabled_ = !g_console_plane.enabled_; },
    [] { return true; } )

} // namespace rn
