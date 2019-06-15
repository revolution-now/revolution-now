/****************************************************************
**europe.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-06-14.
*
* Description: Implements the Europe port view.
*
*****************************************************************/
#include "europe.hpp"

// Revolution Now
#include "plane.hpp"

namespace rn {

namespace {

//- Outbound ships
//- Inbound ships
//- Ships in dock
//- Dock
//- Units on dock
//- Ship cargo
//- Market commodities
//- Exit button
//- Buttons
//- Message box
//- Stats area (money, tax rate, etc.)

/****************************************************************
** The Europe Plane
*****************************************************************/
struct EuropePlane : public Plane {
  EuropePlane() = default;
  bool enabled() const override { return true; }
  bool covers_screen() const override { return true; }
  void draw( Texture const& tx ) const override {
    // clear_texture_transparent( tx );
    clear_texture_black( tx );
    // wm.draw_layout( tx );
  }
  bool input( input::event_t const& event ) override {
    (void)event;
    return true;
  }
  Plane::e_accept_drag can_drag( input::e_mouse_button button,
                                 Coord origin ) override {
    (void)button;
    (void)origin;
    return Plane::e_accept_drag::swallow;
  }
  void on_drag( input::e_mouse_button button, Coord origin,
                Coord prev, Coord current ) override {
    (void)button;
    (void)origin;
    (void)prev;
    (void)current;
  }
};

EuropePlane g_europe_plane;

} // namespace

Plane* europe_plane() { return &g_europe_plane; }

} // namespace rn
