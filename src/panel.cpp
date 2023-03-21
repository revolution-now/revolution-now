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
#include "co-wait.hpp"
#include "compositor.hpp"
#include "error.hpp"
#include "fathers.hpp"
#include "gui.hpp" // FIXME
#include "menu.hpp"
#include "mini-map.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "roles.hpp"
#include "screen.hpp"
#include "ts.hpp" // FIXME
#include "views.hpp"

// config
#include "config/menu-items.rds.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/ui.rds.hpp"

// ss
#include "ss/land-view.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/turn.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** PanelPlane::Impl
*****************************************************************/
struct PanelPlane::Impl : public Plane {
  SS&                           ss_;
  TS&                           ts_;
  unique_ptr<ui::InvisibleView> view;
  wait_promise<>                w_promise;
  MenuPlane::Deregistrar        eot_click_dereg_;

  static Rect rect() {
    UNWRAP_CHECK( res, compositor::section(
                           compositor::e_section::panel ) );
    return res;
  }

  Rect mini_map_available_rect() const {
    Rect      mini_map_available_rect = rect();
    int const kBorder                 = 4;
    mini_map_available_rect.x += kBorder;
    mini_map_available_rect.y += kBorder;
    mini_map_available_rect.w -= kBorder * 2;
    mini_map_available_rect.h = mini_map_available_rect.w;
    return mini_map_available_rect;
  }

  Impl( SS& ss, TS& ts ) : ss_( ss ), ts_( ts ) {
    // Register menu handlers.
    eot_click_dereg_ = ts.planes.menu().register_handler(
        e_menu_item::next_turn, *this );

    vector<ui::OwningPositionedView> view_vec;

    Rect const mini_map_available = mini_map_available_rect();
    auto       mini_map           = make_unique<MiniMapView>(
        ss, ts, mini_map_available.delta() );
    Coord const mini_map_upper_left =
        centered( mini_map->delta(), mini_map_available );
    view_vec.emplace_back( ui::OwningPositionedView{
        .view  = std::move( mini_map ),
        .coord = mini_map_upper_left.with_new_origin(
            rect().upper_left() ) } );

    auto button_view =
        make_unique<ui::ButtonView>( "Next Turn", [this] {
          w_promise.set_value( {} );
          // Disable the button as soon as it is clicked.
          this->next_turn_button().enable( /*enabled=*/false );
        } );
    button_view->blink( /*enabled=*/true );

    auto  button_size = button_view->delta();
    Coord where       = centered( button_size, rect() );
    where.y           = panel_height() - button_size.h * 2;
    where = Coord{} + ( where - rect().upper_left() );

    ui::OwningPositionedView p_view{
        .view = std::move( button_view ), .coord = where };
    view_vec.emplace_back( std::move( p_view ) );

    view = make_unique<ui::InvisibleView>(
        delta(), std::move( view_vec ) );

    next_turn_button().enable( false );
  }

  bool covers_screen() const override { return false; }

  void advance_state() override { view->advance_state(); }

  static W panel_width() { return rect().w; }
  static H panel_height() { return rect().h; }

  Delta delta() const {
    return { panel_width(), panel_height() };
  }
  Coord origin() const { return rect().upper_left(); };

  ui::ButtonView& next_turn_button() const {
    auto p_view = view->at( 1 );
    return *p_view.view->cast<ui::ButtonView>();
  }

  void draw_some_stats( rr::Renderer& renderer,
                        Coord const   where ) const {
    rr::Typer typer =
        renderer.typer( where, config_ui.dialog_text.normal );

    // First some general stats that are not player specific.
    TurnState const& turn_state = ss_.turn;
    typer.write( "{} {}\n",
                 // FIXME
                 ts_.gui.identifier_to_display_name(
                     refl::enum_value_name(
                         turn_state.time_point.season ) ),
                 turn_state.time_point.year );

    typer.newline();

    maybe<e_nation> const curr_nation =
        player_for_role( ss_, e_player_role::active );
    if( !curr_nation ) return;

    // We have an active player, so print some info about it.
    e_nation            nation        = *curr_nation;
    PlayersState const& players_state = ss_.players;
    UNWRAP_CHECK( player, players_state.players[nation] );

    if( player.new_world_name )
      typer.write( "{}\n", *player.new_world_name );
    typer.write( "Nation:  {}\n", nation );
    typer.write( "Gold:    ${}\n", player.money );
    typer.write( "Tax:     {}%\n",
                 player.old_world.taxes.tax_rate );

    typer.newline();
    typer.write( "Bells:   {}/{}\n", player.fathers.bells,
                 bells_needed_for_next_father( ss_, player ) );
    typer.write( "Crosses: {}\n", player.crosses );

    typer.newline();
    typer.write( "Zoom: {:.4}\n",
                 ss_.land_view.viewport.get_zoom() );

    typer.newline();
    typer.write(
        "Independence:\n   {}\n",
        IGui::identifier_to_display_name( refl::enum_value_name(
            player.revolution_status ) ) );
  }

  void draw( rr::Renderer& renderer ) const override {
    rr::Painter painter = renderer.painter();
    tile_sprite( painter, e_tile::wood_middle, rect() );
    // Render border on left and bottom.
    Rect const r = rect();
    {
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .75 );
      rr::Painter painter = renderer.painter();
      painter.draw_vertical_line(
          r.upper_left(), r.h, config_ui.window.border_darker );
      painter.draw_vertical_line(
          r.upper_left() + Delta{ .w = 1 }, r.h,
          config_ui.window.border_dark );
    }

    view->draw( renderer, origin() );

    Rect const mini_map_rect = mini_map_available_rect();

    Coord p =
        mini_map_rect.lower_left() + Delta{ .w = 8, .h = 8 };
    draw_some_stats( renderer, p );
  }

  e_input_handled input( input::event_t const& event ) override {
    // FIXME: we need a window manager in the panel to avoid du-
    // plicating logic between here and the window module.
    if( input::is_mouse_event( event ) ) {
      UNWRAP_CHECK( pos, input::mouse_position( event ) );
      // Normally we require that the cursor be over this view to
      // send input events, but the one exception is for drag
      // events. If we are receiving a drag event here that means
      // that we started within this view, and so (for a smooth
      // UX) we allow the subsequent drag events to be received
      // even if the cursor runs out of this view in the process.
      if( pos.is_inside( rect() ) ||
          event.holds<input::mouse_drag_event_t>() ) {
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

  // Override Plane.
  e_accept_drag can_drag( input::e_mouse_button,
                          Coord origin ) override {
    if( !origin.is_inside( rect() ) ) return e_accept_drag::no;
    // We're doing this so that the minimap can handle dragging
    // without adding the special drag methods to the interface.
    // If we need to change this then we will need to fix the
    // minimap.
    return e_accept_drag::yes_but_raw;
  }

  wait<> user_hits_eot_button() {
    next_turn_button().enable( /*enabled=*/true );
    // Use a scoped setter here so that the button gets disabled
    // if this coroutine gets cancelled.
    SCOPE_EXIT( next_turn_button().enable( /*enabled=*/false ) );
    w_promise.reset();
    co_await w_promise.wait();
  }

  bool will_handle_menu_click( e_menu_item item ) override {
    CHECK( item == e_menu_item::next_turn );
    return next_turn_button().enabled();
  }

  void handle_menu_click( e_menu_item item ) override {
    CHECK( item == e_menu_item::next_turn );
    w_promise.set_value_emplace_if_not_set();
  }

  wait<> wait_for_eot_button_click() {
    return user_hits_eot_button();
  }
};

/****************************************************************
** Menu Handlers
*****************************************************************/
// MENU_ITEM_HANDLER(
//     next_turn,
//     [] {
//       g_panel_plane.w_promise.set_value_emplace_if_not_set();
//     },
//     [] { return g_panel_plane.next_turn_button().enabled(); }
//     )

/****************************************************************
** PanelPlane
*****************************************************************/
Plane& PanelPlane::impl() { return *impl_; }

PanelPlane::~PanelPlane() = default;

PanelPlane::PanelPlane( SS& ss, TS& ts )
  : impl_( new Impl( ss, ts ) ) {}

wait<> PanelPlane::wait_for_eot_button_click() {
  return impl_->wait_for_eot_button_click();
}

} // namespace rn
