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
#include "game-state.hpp"
#include "gs-players.hpp"
#include "gs-turn.hpp"
#include "logger.hpp"
#include "menu.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "views.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** PanelPlane::Impl
*****************************************************************/
struct PanelPlane::Impl : public Plane {
  MenuPlane::Deregistrar eot_click_dereg_;

  Impl( MenuPlane& menu_plane ) : menu_plane_( menu_plane ) {
    // Register menu handlers.
    eot_click_dereg_ = menu_plane_.register_handler(
        e_menu_item::next_turn, *this );

    vector<ui::OwningPositionedView> view_vec;

    auto button_view =
        make_unique<ui::ButtonView>( "Next Turn", [this] {
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

  ui::ButtonView& next_turn_button() const {
    auto p_view = view->at( 0 );
    return *p_view.view->cast<ui::ButtonView>();
  }

  string season_str( e_season season ) const {
    switch( season ) {
      case e_season::winter: return "Winter";
      case e_season::spring: return "Spring";
      case e_season::summer: return "Summer";
      case e_season::fall: return "Autumn";
    }
  }

  void draw_some_stats( rr::Renderer& renderer,
                        Coord const   where ) const {
    Coord p = where;

    rr::Typer typer =
        renderer.typer( where, gfx::pixel::banana() );

    // First some general stats that are not player specific.
    TurnState const& turn_state = GameState::turn();
    typer.write( "{} {}\n",
                 season_str( turn_state.time_point.season ),
                 turn_state.time_point.year );

    typer.newline();

    maybe<NationTurnState const&> nat_st = turn_state.nation;
    if( !nat_st ) return;

    // We have an active player, so print some info about it.
    e_nation            nation        = nat_st->nation;
    PlayersState const& players_state = GameState::players();
    UNWRAP_CHECK(
        player, base::lookup( players_state.players, nation ) );

    if( player.discovered_new_world )
      typer.write( "{}\n", *player.discovered_new_world );
    typer.write( "Nation:  {}\n", nation );
    typer.write( "Gold:    ${}\n", player.money );
    typer.write( "Tax:     {}%\n",
                 player.old_world.taxes.tax_rate );

    typer.newline();

    typer.write( "Crosses: {}\n", player.crosses );
  }

  void draw( rr::Renderer& renderer ) const override {
    rr::Painter painter = renderer.painter();
    tile_sprite( painter, e_tile::wood_middle, rect() );
    view->draw( renderer, origin() );
    Coord p = rect().upper_left() + 44_h + 8_w;
    draw_some_stats( renderer, p );
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

  wait<> user_hits_eot_button() {
    next_turn_button().enable( /*enabled=*/true );
    // Use a scoped setter here so that the button gets disabled
    // if this coroutine gets cancelled.
    SCOPE_EXIT( next_turn_button().enable( /*enabled=*/false ) );
    w_promise = {};
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

  MenuPlane&                    menu_plane_;
  unique_ptr<ui::InvisibleView> view;
  wait_promise<>                w_promise;
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

PanelPlane::PanelPlane( MenuPlane& menu_plane )
  : impl_( new Impl( menu_plane ) ) {}

wait<> PanelPlane::wait_for_eot_button_click() {
  return impl_->wait_for_eot_button_click();
}

} // namespace rn
