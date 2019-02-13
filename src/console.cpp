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
    render_fill_rect( tx, Color{0, 0, 255, 64}, rect );
  }

  bool enabled_{false};
};

ConsolePlane g_console_plane;

} // namespace

Plane* console_plane() { return &g_console_plane; }

MENU_ITEM_HANDLER(
    e_menu_item::toggle_console,
    [] { g_console_plane.enabled_ = !g_console_plane.enabled_; },
    [] { return true; } )

} // namespace rn
