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
    auto frame_rate =
        fmt::format( "fps: {:.1f}", avg_frame_rate() );
    auto frame_rate_tx = render_text_line_shadow(
        fonts::standard, Color::white(), frame_rate );
    copy_texture( frame_rate_tx, tx,
                  rect.lower_right() - frame_rate_tx.size() );
  }

  bool enabled_{false};
};

ConsolePlane g_console_plane;

} // namespace

Plane* console_plane() { return &g_console_plane; }

MENU_ITEM_HANDLER( e_menu_item::toggle_console,
                   [] {
                     g_console_plane.enabled_ =
                         !g_console_plane.enabled_;
                   },
                   [] { return true; } )

} // namespace rn
