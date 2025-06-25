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
#include "cheat.hpp"
#include "co-wait.hpp"
#include "commodity.hpp"
#include "error.hpp"
#include "fathers.hpp"
#include "iengine.hpp"
#include "igui.hpp"
#include "imenu-server.hpp"
#include "land-view.hpp"
#include "map-square.hpp"
#include "mini-map.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "player-mgr.hpp"
#include "roles.hpp"
#include "screen.hpp"
#include "screen.hpp" // FIXME
#include "spread-builder.hpp"
#include "spread-render.hpp"
#include "tiles.hpp"
#include "ts.hpp" // FIXME
#include "unit-mgr.hpp"
#include "views.hpp"

// config
#include "config/menu-items.rds.hpp"
#include "config/text.rds.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/ui.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/land-view.rds.hpp"
#include "ss/old-world-state.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/turn.hpp"
#include "ss/units.hpp"

// render
#include "render/renderer.hpp"
#include "render/typer.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

// FIXME FIXME FIXME
//
// The minimap is a huge performance drain since it is redrawn
// each frame... it causes framerate in debug mode to drop to 30
// fps just with it being enabled... this needs to be improved.
//
// FIXME FIXME FIXME

using ::gfx::point;

/****************************************************************
** PanelPlane::Impl
*****************************************************************/
struct PanelPlane::Impl : public IPlane {
  IEngine& engine_;
  SS& ss_;
  TS& ts_;
  ILandViewPlane const& land_view_plane_;
  unique_ptr<ui::InvisibleView> view;
  wait_promise<> w_promise;
  vector<IMenuServer::Deregistrar> dereg_;

  Rect rect() const {
    auto const r = main_window_logical_rect(
        engine_.video(), engine_.window(),
        engine_.resolutions() );
    return r
        .with_new_left_edge( r.right() - config_ui.panel.width )
        .with_new_top_edge( config_ui.menus.menu_bar_height );
  }

  Rect mini_map_available_rect() const {
    Rect mini_map_available_rect = rect();
    int const kBorder            = 4;
    mini_map_available_rect.x += kBorder;
    mini_map_available_rect.y += 0;
    mini_map_available_rect.w -= kBorder * 2;
    mini_map_available_rect.h = mini_map_available_rect.w;
    return mini_map_available_rect;
  }

  Impl( IEngine& engine, SS& ss, TS& ts,
        ILandViewPlane& land_view_plane )
    : engine_( engine ),
      ss_( ss ),
      ts_( ts ),
      land_view_plane_( land_view_plane ) {
    // Register menu handlers.
    dereg_.push_back(
        ts.planes.get().menu.typed().register_handler(
            e_menu_item::next_turn, *this ) );

    vector<ui::OwningPositionedView> view_vec;

    Rect const mini_map_available = mini_map_available_rect();
    auto mini_map                 = make_unique<MiniMapView>(
        ss, ts, land_view_plane.viewport(),
        mini_map_available.delta() );
    Coord const mini_map_upper_left =
        centered( mini_map->delta(), mini_map_available );
    view_vec.emplace_back( ui::OwningPositionedView{
      .view  = std::move( mini_map ),
      .coord = mini_map_upper_left.with_new_origin(
          rect().upper_left() ) } );

    auto button_view = make_unique<ui::ButtonView>(
        engine_.textometer(), "Next Turn", [this] {
          w_promise.set_value( {} );
          // Disable the button as soon as it is clicked.
          this->next_turn_button().enable( /*enabled=*/false );
        } );
    button_view->blink( /*enabled=*/true );

    auto button_size = button_view->delta();
    Coord where      = centered( button_size, rect() );
    where.y          = panel_height() - button_size.h * 2;
    where            = Coord{} + ( where - rect().upper_left() );

    ui::OwningPositionedView p_view{
      .view = std::move( button_view ), .coord = where };
    view_vec.emplace_back( std::move( p_view ) );

    view = make_unique<ui::InvisibleView>(
        delta(), std::move( view_vec ) );

    next_turn_button().enable( false );
  }

  void advance_state() override { view->advance_state(); }

  W panel_width() const { return rect().w; }
  H panel_height() const { return rect().h; }

  Delta delta() const {
    return { panel_width(), panel_height() };
  }
  Coord origin() const { return rect().upper_left(); };

  ui::ButtonView& next_turn_button() const {
    auto p_view = view->at( 1 );
    return *p_view.view->cast<ui::ButtonView>();
  }

  void draw_some_stats( rr::Renderer& renderer,
                        Coord const where ) const {
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

    maybe<e_player> const curr_player =
        player_for_role( ss_, e_player_role::active );
    if( !curr_player ) return;

    // We have an active player, so print some info about it.
    e_player const player_type        = *curr_player;
    PlayersState const& players_state = ss_.players;
    UNWRAP_CHECK( player, players_state.players[player_type] );

    if( player.new_world_name )
      typer.write( "{}\n", *player.new_world_name );
    typer.write(
        "Gold: {}{}  Tax: {}%\n", player.money,
        config_text.special_chars.currency,
        old_world_state( ss_, player.type ).taxes.tax_rate );

    auto const write_tile = [&]( point const p ) {
      typer.write( "Square: ({}, {})\n", p.x + 1, p.y + 1 );
    };

    auto const write_terrain = [&]( point const p ) {
      string contents;
      MapSquare const& square = ss_.terrain.square_at( p );
      if( square.surface == e_surface::water &&
          square.sea_lane ) {
        contents = "Sea Lane";
      } else {
        e_terrain const terrain = effective_terrain( square );
        contents = IGui::identifier_to_display_name(
            base::to_str( terrain ) );
        if( has_forest( square ) ) contents += " Forest";
      }
      typer.write( "({})\n", contents );
    };

    // Active unit info.
    // FIXME: this needs to persist while the unit is sliding.
    // Probably requires reworking how we decide to display this
    // info. What we might have to do is change is so that the
    // land view pushes this info to the panel instead of the
    // panel detecting it and pulling it.
    if( auto const unit_id = land_view_plane_.unit_blinking();
        unit_id.has_value() ) {
      typer.newline();
      point const p =
          coord_for_unit_indirect_or_die( ss_.units, *unit_id );
      Unit const& unit = ss_.units.unit_for( *unit_id );
      typer.write( "Unit: {}\n", unit.desc().name );
      if( unit.type() == e_unit_type::treasure )
        typer.write( "Treasure: {}{}\n",
                     unit.composition()
                         .inventory()[e_unit_inventory::gold],
                     config_text.special_chars.currency );
      typer.write( "Moves: {}\n", unit.movement_points() );
      write_tile( p );
      write_terrain( p );
      typer.write( "With: " );
      point const spread_origin = typer.position();
      vector<pair<Commodity, int /*quantity*/>> const
          commodities = unit.cargo().commodities();
      vector<TileWithOptions> tiles;
      tiles.reserve( commodities.size() );
      for( auto const& [comm, _] : commodities )
        tiles.push_back( TileWithOptions{
          .tile   = tile_for_commodity_16( comm.type ),
          .greyed = comm.quantity < 100 } );
      InhomogeneousTileSpreadConfig const spread_config{
        .tiles = std::move( tiles ),
        // TODO: need to add appropriate margins here.
        .options    = { .bounds = ( rect().right_edge() -
                                 spread_origin.x ) },
        .sort_tiles = true };
      TileSpreadRenderPlan const spread_plan =
          build_inhomogeneous_tile_spread( engine_.textometer(),
                                           spread_config );
      draw_rendered_icon_spread( renderer, spread_origin,
                                 spread_plan );
    }

    // White box info.
    if( auto const box = land_view_plane_.white_box();
        box.has_value() ) {
      typer.newline();
      write_tile( *box );
      write_terrain( *box );
      // TODO

      // TODO: when the white box is over a foreign player (or
      // REF), it looks like the game does not give full info on
      // how many units are on the tile. We may want to replicate
      // this assuming it is true, though maybe the suppression
      // should be turned off in cheat mode.
    }

    // Extra debugging.
    typer.newline();
    typer.write( "Royal Money: {}{}\n", player.royal_money,
                 config_text.special_chars.currency );
    typer.write( "Zoom: {}\n", ss_.land_view.viewport.zoom );
  }

  void draw( rr::Renderer& renderer ) const override {
    tile_sprite( renderer, e_tile::wood_middle, rect() );
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
    if( event.holds<input::cheat_event_t>() ) {
      enable_cheat_mode( ss_, ts_ );
      return e_input_handled::yes;
    }

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
            mouse_origin_moved_by( event, origin() - Coord{} );
        (void)view->input( new_event );
        return e_input_handled::yes;
      }
      return e_input_handled::no;
    } else
      return view->input( event ) ? e_input_handled::yes
                                  : e_input_handled::no;
  }

  // Override IPlane.
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
    SCOPE_EXIT {
      next_turn_button().enable( /*enabled=*/false );
    };
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

  void on_logical_resolution_selected(
      gfx::e_resolution ) override {}
};

/****************************************************************
** PanelPlane
*****************************************************************/
IPlane& PanelPlane::impl() { return *impl_; }

PanelPlane::~PanelPlane() = default;

PanelPlane::PanelPlane( IEngine& engine, SS& ss, TS& ts,
                        ILandViewPlane& land_view_plane )
  : impl_( new Impl( engine, ss, ts, land_view_plane ) ) {}

wait<> PanelPlane::wait_for_eot_button_click() {
  return impl_->wait_for_eot_button_click();
}

} // namespace rn
