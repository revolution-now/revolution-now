/****************************************************************
**panel.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-11-01.
*
* Description: The side panel on land view.
*
*****************************************************************/
#include "panel.hpp"

// Revolution Now
#include "compositor.hpp"
#include "error.hpp"
#include "gfx.hpp"
#include "logging.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "views.hpp"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** Panel Rendering
*****************************************************************/
struct PanelPlane : public Plane {
  PanelPlane() = default;
  bool covers_screen() const override { return false; }

  static auto rect() {
    UNWRAP_CHECK( res, compositor::section(
                           compositor::e_section::panel ) );
    return res;
  }
  static W panel_width() { return rect().w; }
  static H panel_height() { return rect().h; }

  Delta delta() const {
    return { panel_width(), panel_height() };
  }
  Coord origin() const { return rect().upper_left(); };

  ui::ButtonView& next_turn_button() {
    auto p_view = view->at( 0 );
    return *p_view.view->cast<ui::ButtonView>();
  }

  void initialize() override {
    vector<ui::OwningPositionedView> view_vec;

    auto button_view =
        make_unique<ui::ButtonView>( "Next Turn", [this] {
          lg.debug( "on to next turn." );
          w_promise.set_value( {} );
          // Disable the button as soon as it is clicked.
          this->next_turn_button().enable( /*enabled=*/false );
        } );
    button_view->blink( /*enabled=*/true );

    auto button_size = button_view->delta();
    auto where =
        Coord{} + ( panel_width() / 2 ) - ( button_size.w / 2 );
    where += 16_h;

    ui::OwningPositionedView p_view( std::move( button_view ),
                                     where );
    view_vec.emplace_back( std::move( p_view ) );

    view = make_unique<ui::InvisibleView>(
        delta(), std::move( view_vec ) );

    next_turn_button().enable( false );
  }

  void draw( Texture& tx ) const override {
    clear_texture_transparent( tx );
    auto left_side =
        0_x + main_window_logical_size().w - panel_width();

    auto const& wood = lookup_sprite( e_tile::wood_middle );
    auto        wood_width  = wood.size().w;
    auto        wood_height = wood.size().h;

    for( Y i = rect().top_edge(); i < rect().bottom_edge();
         i += wood_height )
      render_sprite( tx, e_tile::wood_middle, i,
                     left_side + wood_width, 0, 0 );
    for( Y i = rect().top_edge(); i < rect().bottom_edge();
         i += wood_height )
      render_sprite( tx, e_tile::wood_left_edge, i, left_side, 0,
                     0 );

    view->draw( tx, origin() );
  }

  e_input_handled input( input::event_t const& event ) override {
    // FIXME: we need a window manager in the panel to avoid du-
    // plicating logic between here and the window module.
    if( input::is_mouse_event( event ) ) {
      UNWRAP_CHECK( pos, input::mouse_position( event ) );
      if( pos.is_inside( rect() ) ) {
        auto new_event =
            move_mouse_origin_by( event, origin() - Coord{} );
        (void)view->input( new_event );
        return e_input_handled::yes;
      }
      return e_input_handled::no;
    } else
      return view->input( event ) ? e_input_handled::yes
                                  : e_input_handled::no;
  }

  waitable<> user_hits_eot_button() {
    next_turn_button().enable( /*enabled=*/true );
    w_promise = {};
    return w_promise.get_waitable();
  }

  unique_ptr<ui::InvisibleView> view;
  waitable_promise<>            w_promise;
};

PanelPlane g_panel_plane;

} // namespace

/****************************************************************
** Public API
*****************************************************************/
Plane* panel_plane() { return &g_panel_plane; }

waitable<> user_hits_eot_button() {
  return g_panel_plane.user_hits_eot_button();
}

/****************************************************************
** Testing
*****************************************************************/
void test_panel() {
  //
}

} // namespace rn
