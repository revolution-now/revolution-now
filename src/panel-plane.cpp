/****************************************************************
**panel-plane.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-11-01.
*
* Description: The side panel on land view.
*
*****************************************************************/
#include "panel-plane.hpp"

// Revolution Now
#include "cheat.hpp"
#include "co-wait.hpp"
#include "commodity.hpp"
#include "error.hpp"
#include "fathers.hpp"
#include "iengine.hpp"
#include "igui.hpp"
#include "imenu-handler.hpp"
#include "imenu-server.hpp"
#include "land-view.hpp"
#include "map-square.hpp"
#include "mini-map.hpp"
#include "panel.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "player-mgr.hpp"
#include "render.hpp"
#include "revolution-status.hpp"
#include "roles.hpp"
#include "screen.hpp"
#include "screen.hpp" // FIXME
#include "spread-builder.hpp"
#include "spread-render.hpp"
#include "tiles.hpp"
#include "trade-route.hpp"
#include "tribe-mgr.hpp"
#include "ts.hpp" // FIXME
#include "unit-flag.hpp"
#include "unit-mgr.hpp"
#include "views.hpp"
#include "visibility.hpp"

// config
#include "config/menu-items.rds.hpp"
#include "config/nation.rds.hpp"
#include "config/natives.rds.hpp"
#include "config/text.rds.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/ui.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
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

// rds
#include "rds/switch-macro.hpp"

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
using ::gfx::rect;
using ::gfx::size;

/****************************************************************
** PanelPlane::Impl
*****************************************************************/
struct PanelPlane::Impl : public IPlane, public IMenuHandler {
  IEngine& engine_;
  SS& ss_;
  TS& ts_;
  ILandViewPlane const& land_view_plane_;
  unique_ptr<ui::InvisibleView> view_;
  wait_promise<> w_promise_;
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
    int const kBorder            = 8;
    mini_map_available_rect.x += kBorder;
    mini_map_available_rect.y += kBorder / 2;
    mini_map_available_rect.w -= kBorder * 2;
    // This mirrors the OG's ratio, which appears to be a good
    // one both in terms of function and appearance.
    mini_map_available_rect.h =
        static_cast<int>( mini_map_available_rect.h / 4.5 );
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
          w_promise_.set_value( {} );
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

    view_ = make_unique<ui::InvisibleView>(
        delta(), std::move( view_vec ) );

    next_turn_button().enable( false );
  }

  void advance_state() override { view_->advance_state(); }

  W panel_width() const { return rect().w; }
  H panel_height() const { return rect().h; }

  Delta delta() const {
    return { panel_width(), panel_height() };
  }
  Coord origin() const { return rect().upper_left(); };

  ui::ButtonView& next_turn_button() const {
    auto p_view = view_->at( 1 );
    return *p_view.view->cast<ui::ButtonView>();
  }

  void render_divider( rr::Renderer& renderer, point const pos,
                       int const max_width ) const {
    int const width     = max_width - 8;
    rr::Painter painter = renderer.painter();
    painter.draw_horizontal_line( pos + size{ .w = 1, .h = -1 },
                                  width + 2,
                                  config_ui.window.border_dark );
    painter.draw_horizontal_line(
        pos + size{ .w = ( width * 3 ) / 4 + 7, .h = -1 },
        width / 3 - 12, config_ui.window.border_light );
    painter.draw_horizontal_line(
        pos, width, config_ui.window.border_darker );
    painter.draw_horizontal_line(
        pos + size{ .w = ( width * 3 ) / 4 + 4 }, width / 3 - 9,
        config_ui.window.border_dark );
    painter.draw_horizontal_line(
        pos + size{ .w = 1, .h = +1 }, width + 3,
        config_ui.window.border_light );
    painter.draw_horizontal_line(
        pos + size{ .w = ( width * 3 ) / 4, .h = +1 },
        width / 3 - 5, config_ui.window.border_lighter );
  }

  void draw_some_stats( rr::Renderer& renderer,
                        Coord const where ) const {
    rr::Typer typer =
        renderer.typer( where, config_ui.dialog_text.normal );

    // First some general stats that are not player specific.
    TurnState const& turn_state = ss_.turn;
    typer.write( "{} {}\n",
                 ts_.gui.identifier_to_display_name(
                     refl::enum_value_name(
                         turn_state.time_point.season ) ),
                 turn_state.time_point.year );

    auto const viewer =
        player_for_role( ss_, e_player_role::viewer );
    auto const viz = create_visibility_for( ss_, viewer );
    PanelEntities const entities =
        entities_shown_on_panel( ss_ );

    if( !entities.player.has_value() ) return;

    // We have an active player, so print some info about it.
    e_player const player_type        = *entities.player;
    PlayersState const& players_state = ss_.players;
    UNWRAP_CHECK( player, players_state.players[player_type] );

    typer.write(
        "Gold: {}{}  Tax: {}%\n", player.money,
        config_text.special_chars.currency,
        old_world_state( ss_, player.type ).taxes.tax_rate );

    if constexpr( false ) {
      if( player.new_world_name )
        typer.write( "{}\n", *player.new_world_name );
    }

    auto const write_tile = [&] {
      if( !entities.tile.has_value() ) return;
      typer.write( "Tile: ({}, {})\n", entities.tile->x + 1,
                   entities.tile->y + 1 );
    };

    auto const write_terrain = [&] {
      if( !entities.square.has_value() ) return;
      string contents;
      MapSquare const& square = *entities.square;
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
    if( auto const active_unit = entities.active_unit;
        active_unit.has_value() ) {
      typer.newline();
      UnitId const unit_id = active_unit->unit_id;
      Unit const& unit     = ss_.units.unit_for( unit_id );
      UnitFlagRenderInfo const flag_info =
          euro_unit_flag_render_info(
              unit, viewer,
              UnitFlagOptions{ .flag_count =
                                   e_flag_count::single } );
      render_unit( renderer, typer.position(), unit,
                   UnitRenderOptions{ .flag = flag_info } );
      int const shift = 32 + 6;
      typer.move_frame_by( { .w = shift } );
      typer.write( "Moves: {}\n", unit.movement_points() );
      write_tile();
      typer.newline();
      typer.newline();
      typer.move_frame_by( { .w = -shift } );

      typer.write( "{} {}\n", player_possessive( player ),
                   unit.desc().name );
      if( unit.type() == e_unit_type::treasure )
        typer.write( "Holding: {}{}\n",
                     unit.composition()
                         .inventory()[e_unit_inventory::gold],
                     config_text.special_chars.currency );
      typer.set_color( config_ui.dialog_text.highlighted );
      typer.write( "{}\n", active_unit->orders_name );
      typer.set_color( config_ui.dialog_text.normal );
      write_terrain();
      typer.newline();
      vector<pair<Commodity, int /*slot*/>> const commodities =
          unit.cargo().commodities();
      if( !commodities.empty() ) {
        typer.write( "With: " );
        point const spread_origin = typer.position();
        vector<TileWithOptions> tiles;
        tiles.reserve( commodities.size() );
        for( auto const& [comm, _] : commodities )
          tiles.push_back( TileWithOptions{
            .tile   = tile_for_commodity_16( comm.type ),
            .greyed = comm.quantity < 100 } );
        InhomogeneousTileSpreadConfig const spread_config{
          .tiles = std::move( tiles ),
          .options    = { .bounds = ( rect().right_edge() -
                                   spread_origin.x ) },
          .sort_tiles = true };
        TileSpreadRenderPlan const spread_plan =
            build_inhomogeneous_tile_spread(
                engine_.textometer(), spread_config );
        draw_rendered_icon_spread( renderer, spread_origin,
                                   spread_plan );
      }
      typer.newline();
    } else {
      // No active unit.
      write_tile();
      write_terrain();
    }

    if( entities.city.has_value() ) {
      UNWRAP_CHECK_T( point const tile, entities.tile );
      auto const write_colony = [&]( Colony const& colony ) {
        e_tile const sprite_tile =
            houses_tile_for_colony( colony );
        gfx::rect const trimmed =
            trimmed_area_for( sprite_tile );
        render_colony(
            renderer,
            typer.position() -
                trimmed.origin.distance_from_origin(),
            *viz, tile, ss_, colony,
            ColonyRenderOptions{ .render_name       = false,
                                 .render_population = false,
                                 .render_flag       = true } );
        int const shift = trimmed.size.w + 6;
        typer.move_frame_by( { .w = shift } );
        typer.write( "{}\n", colony.name );
        typer.newline();
        typer.newline();
        typer.newline();
        typer.move_frame_by( { .w = -shift } );
        typer.newline();
      };
      SWITCH( *entities.city ) {
        CASE( dwelling ) {
          UNWRAP_CHECK_T( Dwelling const& dwelling_o,
                          viz->dwelling_at( *entities.tile ) );
          e_tile const sprite_tile =
              tile_for_dwelling( ss_, dwelling_o );
          gfx::rect const trimmed =
              trimmed_area_for( sprite_tile );
          render_dwelling(
              renderer,
              typer.position() -
                  trimmed.origin.distance_from_origin(),
              *viz, tile, ss_, dwelling_o );
          e_tribe const tribe_type =
              tribe_type_for_dwelling( ss_, dwelling_o );
          string const tribe_name =
              config_natives.tribes[tribe_type].name_possessive;
          int const shift = trimmed.size.w + 6;
          typer.move_frame_by( { .w = shift } );
          typer.write( "{}\n", tribe_name );
          typer.write(
              "{}\n",
              config_natives
                  .dwelling_types
                      [config_natives.tribes[tribe_type].level]
                  .name_singular );
          typer.newline();
          typer.newline();
          typer.move_frame_by( { .w = -shift } );
          break;
        }
        CASE( foreign_colony ) {
          UNWRAP_CHECK_T( Colony const& colony,
                          viz->colony_at( *entities.tile ) );
          write_colony( colony );
          break;
        }
        CASE( visible_colony ) {
          Colony const& colony = ss_.colonies.colony_for(
              visible_colony.colony_id );
          write_colony( colony );
          vector<Commodity> const commodities =
              colony_commodities_by_value( ss_, player, colony );
          if( !commodities.empty() ) {
            typer.write( "With: " );
            point const spread_origin = typer.position();
            vector<TileWithOptions> tiles;
            tiles.reserve( commodities.size() );
            for( int i = 0; auto const& comm : commodities ) {
              if( comm.quantity == 0 ) continue;
              if( i++ == 5 ) break;
              tiles.push_back( TileWithOptions{
                .tile   = tile_for_commodity_16( comm.type ),
                .greyed = comm.quantity < 100 } );
            }
            InhomogeneousTileSpreadConfig const spread_config{
              .tiles = std::move( tiles ),
              .options    = { .bounds = ( rect().right_edge() -
                                       spread_origin.x ) },
              .sort_tiles = true };
            TileSpreadRenderPlan const spread_plan =
                build_inhomogeneous_tile_spread(
                    engine_.textometer(), spread_config );
            draw_rendered_icon_spread( renderer, spread_origin,
                                       spread_plan );
            break;
          }
        }
      }
      typer.newline();
    }

    typer.newline();
    if( !entities.unit_stack.empty() ) {
      for( PanelUnit const& punit : entities.unit_stack ) {
        switch( ss_.units.unit_kind( punit.generic_unit_id ) ) {
          case e_unit_kind::native: {
            NativeUnit const& native_unit =
                ss_.units.native_unit_for(
                    punit.generic_unit_id );
            UnitFlagRenderInfo const flag_info =
                native_unit_flag_render_info(
                    ss_, native_unit,
                    UnitFlagOptions{
                      .flag_count =
                          entities.has_multiple_units
                              ? e_flag_count::multiple
                              : e_flag_count::single } );
            render_native_unit(
                renderer, typer.position(), native_unit,
                UnitRenderOptions{ .flag = flag_info } );
            e_tribe const tribe_type =
                tribe_type_for_unit( ss_, native_unit );
            string const tribe_name =
                config_natives.tribes[tribe_type]
                    .name_possessive;
            int const shift = 32 + 6;
            typer.move_frame_by( { .w = shift } );
            typer.write( "{}\n", tribe_name );
            typer.write(
                "{}\n",
                config_natives.unit_types[native_unit.type]
                    .name_plural );
            typer.newline();
            typer.newline();
            typer.move_frame_by( { .w = -shift } );
            break;
          }
          case e_unit_kind::euro: {
            Unit const& unit =
                ss_.units.euro_unit_for( punit.generic_unit_id );
            UnitFlagRenderInfo const flag_info =
                euro_unit_flag_render_info(
                    unit, viewer,
                    UnitFlagOptions{
                      .flag_count =
                          ( entities.has_multiple_units &&
                            entities.unit_stack.size() == 1 )
                              ? e_flag_count::multiple
                              : e_flag_count::single } );
            render_unit(
                renderer, typer.position(), unit,
                UnitRenderOptions{ .flag = flag_info } );
            int const shift = 32 + 6;
            typer.move_frame_by( { .w = shift } );

            typer.set_color( config_ui.dialog_text.highlighted );
            if( unit.type() == e_unit_type::treasure )
              typer.write(
                  "Holding: {}{}\n",
                  unit.composition()
                      .inventory()[e_unit_inventory::gold],
                  config_text.special_chars.currency );

            vector<pair<Commodity, int /*slot*/>> const
                commodities = unit.cargo().commodities();
            if( !commodities.empty() ) {
              point const spread_origin = typer.position();
              vector<TileWithOptions> tiles;
              tiles.reserve( commodities.size() );
              for( auto const& [comm, _] : commodities )
                tiles.push_back( TileWithOptions{
                  .tile   = tile_for_commodity_16( comm.type ),
                  .greyed = comm.quantity < 100 } );
              InhomogeneousTileSpreadConfig const spread_config{
                .tiles = std::move( tiles ),
                .options    = { .bounds = ( rect().right_edge() -
                                         spread_origin.x ) },
                .sort_tiles = true };
              TileSpreadRenderPlan const spread_plan =
                  build_inhomogeneous_tile_spread(
                      engine_.textometer(), spread_config );
              draw_rendered_icon_spread( renderer, spread_origin,
                                         spread_plan );
            }
            typer.newline();
            typer.newline();
            typer.write( "{}\n", punit.orders_name );
            typer.set_color( config_ui.dialog_text.normal );
            typer.move_frame_by( { .w = -shift } );
            break;
          }
        }
      }
    }

    // Extra debugging.
    typer.newline();
    typer.write( "Royal Money: {}{}\n", player.royal_money,
                 config_text.special_chars.currency );
    typer.write( "Zoom: {}\n", ss_.land_view.viewport.zoom );

    // Bottom.
    {
      size const padding = { .w = 8, .h = -4 };
      int const height   = 4;
      gfx::rect const curr_player_rect{
        .origin = rect().to_gfx().sw() + size{ .h = -height } +
                  padding,
        .size = { .w = rect().to_gfx().size.w - padding.w * 2,
                  .h = height } };
      renderer.painter().draw_solid_rect(
          curr_player_rect,
          config_nation.players[player_type].flag_color );
    }
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

    view_->draw( renderer, origin() );

    Rect const mini_map_rect = mini_map_available_rect();

    Coord const divider_p = mini_map_rect.to_gfx()
                                .sw()
                                .with_x( rect().x )
                                .moved_down( 8 );
    render_divider( renderer, divider_p, rect().w );

    Coord const p =
        mini_map_rect.lower_left() + Delta{ .h = 8 + 6 };
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
        (void)view_->input( new_event );
        return e_input_handled::yes;
      }
      return e_input_handled::no;
    } else
      return view_->input( event ) ? e_input_handled::yes
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
    w_promise_.reset();
    co_await w_promise_.wait();
  }

  bool will_handle_menu_click( e_menu_item item ) override {
    CHECK( item == e_menu_item::next_turn );
    return next_turn_button().enabled();
  }

  void handle_menu_click( e_menu_item item ) override {
    CHECK( item == e_menu_item::next_turn );
    w_promise_.set_value_emplace_if_not_set();
  }

  wait<> wait_for_eot_button_click() {
    return user_hits_eot_button();
  }

  void on_logical_resolution_selected(
      gfx::e_resolution ) override {
    vector<ui::OwningPositionedView> view_vec;
    Rect const mini_map_available = mini_map_available_rect();
    auto mini_map                 = make_unique<MiniMapView>(
        ss_, ts_, land_view_plane_.viewport(),
        mini_map_available.delta() );
    Coord const mini_map_upper_left =
        centered( mini_map->delta(), mini_map_available );
    view_vec.emplace_back( ui::OwningPositionedView{
      .view  = std::move( mini_map ),
      .coord = mini_map_upper_left.with_new_origin(
          rect().upper_left() ) } );

    auto button_view = make_unique<ui::ButtonView>(
        engine_.textometer(), "Next Turn", [this] {
          w_promise_.set_value( {} );
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

    view_ = make_unique<ui::InvisibleView>(
        delta(), std::move( view_vec ) );
  }
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
