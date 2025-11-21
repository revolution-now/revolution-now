/****************************************************************
**panel.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-11-15.
*
* Description: Implementation for the panel.
*
*****************************************************************/
#include "panel.hpp"

// Revolution Now
#include "commodity.hpp"
#include "igui.hpp"
#include "map-square.hpp"
#include "player-mgr.hpp"
#include "render.hpp"
#include "revolution-status.hpp"
#include "roles.hpp"
#include "society.hpp"
#include "spread-builder.hpp"
#include "spread-render.hpp"
#include "tiles.hpp"
#include "trade-route.hpp"
#include "tribe-mgr.hpp"
#include "unit-mgr.hpp"
#include "unit-stack.hpp"
#include "visibility.hpp"
#include "white-box.hpp"

// config
#include "config/natives.rds.hpp"
#include "config/text.rds.hpp"
#include "config/ui.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/land-view.rds.hpp"
#include "ss/old-world-state.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/turn.rds.hpp"
#include "ss/units.hpp"

// renderer
#include "render/itextometer.hpp"
#include "render/renderer.hpp"
#include "render/typer.hpp"

// rds
#include "rds/switch-macro.hpp"

namespace rn {

namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

string orders_name_for_euro_unit( SSConst const& ss,
                                  UnitId const unit_id ) {
  Unit const& unit = ss.units.unit_for( unit_id );
  SWITCH( unit.orders() ) {
    CASE( damaged ) {
      // Shouldn't really be here, but oh well...
      return "Damaged";
    }
    CASE( fortified ) { return "Fortified"; }
    CASE( fortifying ) { return "Fortifying"; }
    CASE( go_to ) {
      SWITCH( go_to.target ) {
        CASE( harbor ) {
          return format(
              "Go To {}",
              config_nation
                  .nations[nation_for( unit.player_type() )]
                  .harbor_city_name );
        }
        CASE( map ) {
          return format( "Go To ({}, {})", map.tile.x + 1,
                         map.tile.y + 1 );
        }
      }
    }
    CASE( none ) { return "No Orders"; }
    CASE( plow ) { return "Clear/Plow"; }
    CASE( road ) { return "Build Road"; }
    CASE( sentry ) { return "Sentried"; }
    CASE( trade_route ) { return "Trade Route"; }
  }
}

maybe<string> orders_name_for_unit_generic(
    SSConst const& ss, GenericUnitId const generic_unit_id ) {
  switch( ss.units.unit_kind( generic_unit_id ) ) {
    case e_unit_kind::euro:
      return orders_name_for_euro_unit(
          ss, ss.units.check_euro_unit( generic_unit_id ) );
    case e_unit_kind::native:
      return nothing;
  }
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
PanelEntities entities_shown_on_panel( SSConst const& ss,
                                       IVisibility const& viz ) {
  PanelEntities entities;

  auto const populate_colony = [&]( point const tile ) {
    // If the entire map is visible then there are no constraints
    // on what can be seen, regardless of player/ownership.
    if( !viz.player().has_value() ) {
      auto const colony_id =
          ss.colonies.maybe_from_coord( tile );
      if( !colony_id.has_value() ) return;
      entities.city =
          PanelCity::visible_colony{ .colony_id = *colony_id };
      return;
    }
    auto const colony = viz.colony_at( tile );
    if( !colony.has_value() ) return;
    CHECK( viz.player().has_value() );
    if( colony->player == *viz.player() ) {
      CHECK_GT( colony->id, 0 ); // shall not be fogged.
      entities.city =
          PanelCity::visible_colony{ .colony_id = colony->id };
      return;
    }
    // At this point there is a colony that is visible on the
    // tile, but the map is being viewed by a player that does
    // not own that colony. Thus, the panel should show only a
    // limited amount of info about it regardless of whether the
    // tile is fogged or not. The caller will have to look up the
    // colony using the visibility object.
    entities.city = PanelCity::foreign_colony{};
  };

  auto const populate_dwelling = [&]( point const tile ) {
    if( viz.dwelling_at( tile ).has_value() )
      entities.city = PanelCity::dwelling{};
  };

  auto const populate_unit_stack = [&]( point const tile ) {
    vector<GenericUnitId> const units_sorted = [&] {
      auto const units =
          units_from_coord_recursive( ss.units, tile );
      vector<GenericUnitId> res( units.begin(), units.end() );
      sort_unit_stack( ss, res );
      return res;
    }();
    if( units_sorted.empty() ) return;
    auto const show_all_units = [&] {
      entities.has_multiple_units = units_sorted.size() > 1;
      for( GenericUnitId const generic_unit_id : units_sorted )
        entities.unit_stack.push_back( PanelUnit{
          .generic_unit_id = generic_unit_id,
          .orders_name     = orders_name_for_unit_generic(
              ss, generic_unit_id ) } );
    };
    auto const show_first_unit = [&] {
      CHECK( !units_sorted.empty() );
      entities.has_multiple_units = units_sorted.size() > 1;
      GenericUnitId const generic_unit_id = units_sorted[0];
      entities.unit_stack.push_back(
          PanelUnit{ .generic_unit_id = generic_unit_id,
                     .orders_name = orders_name_for_unit_generic(
                         ss, generic_unit_id ) } );
    };
    // If the entire map is visible then there are no constraints
    // on what can be seen, regardless of player/ownership.
    if( !viz.player().has_value() ) {
      show_all_units();
      return;
    }
    CHECK( viz.player().has_value() );
    VisibleSociety const viz_society =
        society_on_visible_square( ss, viz, tile );
    SWITCH( viz_society ) {
      CASE( hidden ) { return; }
      CASE( empty ) { return; }
      CASE( society ) {
        Society const& soc = society.value;
        SWITCH( soc ) {
          CASE( european ) {
            if( european.player == *viz.player() )
              show_all_units();
            else
              show_first_unit();
            break;
          }
          CASE( native ) {
            show_first_unit();
            break;
          }
        }
        break;
      }
    }
  };

  auto const populate_for_tile = [&]( point const tile ) {
    entities.tile   = tile;
    entities.square = viz.visible_square_at( tile );
    populate_colony( tile );
    populate_dwelling( tile );
    populate_unit_stack( tile );
    return entities;
  };

  auto const populate_for_white_box = [&] {
    populate_for_tile( white_box_tile( ss ) );
  };

  SWITCH( ss.turn.cycle ) {
    CASE( not_started ) { break; }
    CASE( natives ) { break; }
    CASE( player ) {
      UNWRAP_CHECK_T( Player const& player_o,
                      ss.players.players[player.type] );
      if( player_o.control != e_player_control::human ) break;
      if( viz.player().has_value() &&
          *viz.player() != player.type )
        break;
      entities.player_turn = player.type;
      SWITCH( player.st ) {
        CASE( not_started ) { break; }
        CASE( units ) {
          if( units.view_mode ) {
            populate_for_white_box();
            break;
          }
          if( units.q.empty() ) break;
          UnitId const unit_id = units.q.front();
          entities.active_unit = PanelActiveUnit{
            .unit_id = unit_id,
            .orders_name =
                orders_name_for_euro_unit( ss, unit_id ) };
          auto const tile =
              coord_for_unit_multi_ownership( ss, unit_id );
          if( tile.has_value() ) populate_for_tile( *tile );
          break;
        }
        CASE( eot ) {
          populate_for_white_box();
          break;
        }
        CASE( post ) { break; }
        CASE( finished ) { break; }
      }
      break;
    }
    CASE( intervention ) { break; }
    CASE( end_cycle ) { break; }
    CASE( finished ) { break; }
  }

  // Remove the active unit from the unit list if it is there.
  if( entities.active_unit.has_value() )
    erase_if( entities.unit_stack, [&]( PanelUnit const punit ) {
      return punit.generic_unit_id ==
             entities.active_unit->unit_id;
    } );

  // If we have a foreign view of a colony then don't put any
  // units there since it is a foreign colony.
  if( entities.city.holds<PanelCity::foreign_colony>() ) {
    entities.unit_stack.clear();
    entities.has_multiple_units = false;
  }

  return entities;
}

PanelLayout panel_layout( SSConst const& ss,
                          IVisibility const& viz,
                          PanelEntities const& entities ) {
  PanelLayout layout;

  auto const row = [&]( PanelRow&& row ) {
    layout.rows.push_back( std::move( row ) );
  };

  auto const row_with_entity = [&]( auto&& entity ) {
    row( PanelRow{ .columns = { { std::move( entity ) } } } );
  };

  auto const text_row = [&]( string line,
                             bool const highlight = false ) {
    if( highlight )
      row_with_entity( PanelEntity::text_highlight{
        .value = std::move( line ) } );
    else
      row_with_entity(
          PanelEntity::text{ .value = std::move( line ) } );
  };

  auto const blank_line = [&] { text_row( "" ); };

  auto const add_column = [&] {
    if( layout.rows.empty() ) layout.rows.resize( 1 );
    layout.rows.back().columns.emplace_back();
  };

  auto const inner_row = [&]( PanelEntity entity ) {
    if( layout.rows.empty() ) layout.rows.resize( 1 );
    if( layout.rows.back().columns.empty() )
      layout.rows.back().columns.resize( 1 );
    layout.rows.back().columns.back().push_back(
        std::move( entity ) );
  };

  auto const text_row_inner =
      [&]( string line, bool const highlight = false ) {
        if( highlight )
          inner_row( PanelEntity::text_highlight{
            .value = std::move( line ) } );
        else
          inner_row(
              PanelEntity::text{ .value = std::move( line ) } );
      };

  auto const column_with_entity = [&]( PanelEntity entity ) {
    if( layout.rows.empty() ) layout.rows.resize( 1 );
    layout.rows.back().columns.push_back(
        { std::move( entity ) } );
  };

  auto const column_with_text = [&]( string const& text,
                                     bool const highlight =
                                         false ) {
    if( highlight )
      column_with_entity(
          PanelEntity::text_highlight{ .value = text } );
    else
      column_with_entity( PanelEntity::text{ .value = text } );
  };

  // First some general stats that are not player specific.
  TurnState const& turn_state = ss.turn;
  text_row( format(
      "{} {}",
      IGui::identifier_to_display_name( refl::enum_value_name(
          turn_state.time_point.season ) ),
      turn_state.time_point.year ) );

  if( !entities.player_turn.has_value() ) return layout;

  // We have an active player, so print some info about it.
  e_player const player_turn        = *entities.player_turn;
  PlayersState const& players_state = ss.players;
  UNWRAP_CHECK( player, players_state.players[player_turn] );

  text_row( format(
      "Gold: {}{}  Tax: {}%", player.money,
      config_text.special_chars.currency,
      old_world_state( ss, player.type ).taxes.tax_rate ) );

  // FIXME: in the OG land tiles have ownership, and this is
  // only displayed when we own the land. Otherwise it will
  // display which tribe owns the land... perhaps we need to
  // replicate this.  Until then we can't display it...
  if constexpr( false ) {
    if( player.new_world_name )
      text_row( format( "{}", *player.new_world_name ) );
  }

  auto const tile_text = [&] {
    if( !entities.tile.has_value() ) return "Location: (none)"s;
    return format( "Location: ({}, {})", entities.tile->x + 1,
                   entities.tile->y + 1 );
  };

  auto const write_tile = [&] { text_row( tile_text() ); };

  auto const write_tile_inner = [&] {
    text_row_inner( tile_text() );
  };

  auto const write_terrain = [&] {
    if( !entities.square.has_value() ) return;
    string contents;
    MapSquare const& square = *entities.square;
    if( square.surface == e_surface::water && square.sea_lane ) {
      contents = "Sea Lane";
    } else {
      e_terrain const terrain = effective_terrain( square );
      contents                = IGui::identifier_to_display_name(
          base::to_str( terrain ) );
      if( has_forest( square ) ) contents += " Forest";
    }
    text_row( format( "({})", contents ) );
  };

  // Active unit info.
  if( auto const active_unit = entities.active_unit;
      active_unit.has_value() ) {
    blank_line();
    UnitId const unit_id = active_unit->unit_id;
    Unit const& unit     = ss.units.unit_for( unit_id );
    row_with_entity(
        PanelEntity::euro_unit{ .unit_id = unit_id } );
    add_column();
    text_row_inner(
        format( "Moves: {}", unit.movement_points() ) );
    write_tile_inner();
    text_row( format( "{} {}", player_possessive( player ),
                      unit.desc().name ) );
    if( unit.type() == e_unit_type::treasure )
      text_row( format(
          "Holding: {}{}",
          unit.composition().inventory()[e_unit_inventory::gold],
          config_text.special_chars.currency ) );
    row_with_entity( PanelEntity::text_highlight{
      .value = active_unit->orders_name } );
    write_terrain();
    blank_line();
    vector<pair<Commodity, int /*slot*/>> const commodities =
        unit.cargo().commodities();
    if( !commodities.empty() ) {
      text_row( "With:" );
      PanelEntity::commodity_spread spread;
      for( auto const& [comm, _] : commodities )
        spread.comms.push_back( PanelCommodity{
          .type = comm.type, .greyed = comm.quantity < 100 } );
      column_with_entity( std::move( spread ) );
    }
    blank_line();
  } else {
    // No active unit.
    write_tile();
    write_terrain();
  }

  if( entities.city.has_value() ) {
    UNWRAP_CHECK_T( point const tile, entities.tile );
    auto const write_colony = [&]( Colony const& colony ) {
      row_with_entity(
          PanelEntity::icon_colony{ .tile = tile } );
      column_with_text( format( "{}", colony.name ) );
    };
    auto const write_dwelling = [&]( Dwelling const& dwelling ) {
      row_with_entity(
          PanelEntity::icon_dwelling{ .tile = tile } );
      add_column();
      e_tribe const tribe_type =
          tribe_type_for_dwelling( ss, dwelling );
      string const tribe_name =
          config_natives.tribes[tribe_type].name_possessive;
      text_row_inner( format( "{}", tribe_name ) );
      text_row_inner( format(
          "{}",
          config_natives
              .dwelling_types[config_natives.tribes[tribe_type]
                                  .level]
              .name_singular ) );
    };
    SWITCH( *entities.city ) {
      CASE( dwelling ) {
        UNWRAP_CHECK_T( Dwelling const& dwelling_o,
                        viz.dwelling_at( *entities.tile ) );
        write_dwelling( dwelling_o );
        blank_line();
        blank_line();
        break;
      }
      CASE( foreign_colony ) {
        UNWRAP_CHECK_T( Colony const& colony,
                        viz.colony_at( *entities.tile ) );
        write_colony( colony );
        blank_line();
        break;
      }
      CASE( visible_colony ) {
        Colony const& colony =
            ss.colonies.colony_for( visible_colony.colony_id );
        write_colony( colony );
        blank_line();
        vector<Commodity> const commodities =
            colony_commodities_by_value( ss, player, colony );
        if( !commodities.empty() ) {
          text_row( "With: " );
          PanelEntity::commodity_spread spread;
          for( auto const& comm : commodities )
            spread.comms.push_back( PanelCommodity{
              .type   = comm.type,
              .greyed = comm.quantity < 100 } );
          column_with_entity( std::move( spread ) );
          break;
        }
      }
    }
    blank_line();
  }

  blank_line();
  if( !entities.unit_stack.empty() ) {
    for( PanelUnit const& punit : entities.unit_stack ) {
      switch( ss.units.unit_kind( punit.generic_unit_id ) ) {
        case e_unit_kind::native: {
          NativeUnit const& native_unit =
              ss.units.native_unit_for( punit.generic_unit_id );
          row_with_entity( PanelEntity::brave{
            .native_unit_id = native_unit.id,
            .flag_stacked   = entities.has_multiple_units } );
          add_column();
          e_tribe const tribe_type =
              tribe_type_for_unit( ss, native_unit );
          string const tribe_name =
              config_natives.tribes[tribe_type].name_possessive;
          text_row_inner( format( "{}", tribe_name ) );
          text_row_inner( format(
              "{}", config_natives.unit_types[native_unit.type]
                        .name_plural ) );
          blank_line();
          blank_line();
          break;
        }
        case e_unit_kind::euro: {
          Unit const& unit =
              ss.units.euro_unit_for( punit.generic_unit_id );
          row_with_entity( PanelEntity::euro_unit{
            .unit_id = unit.id(),
            .flag_stacked =
                ( entities.has_multiple_units &&
                  entities.unit_stack.size() == 1 ) } );
          add_column();
          text_row_inner( unit.desc().name, /*highlight=*/true );
          if( unit.type() == e_unit_type::treasure )
            text_row_inner(
                format( "Holding: {}{}",
                        unit.composition()
                            .inventory()[e_unit_inventory::gold],
                        config_text.special_chars.currency ),
                /*highlight=*/true );
          vector<pair<Commodity, int /*slot*/>> const
              commodities = unit.cargo().commodities();
          if( !commodities.empty() ) {
            PanelEntity::commodity_spread spread;
            for( auto const& [comm, _] : commodities )
              spread.comms.push_back( PanelCommodity{
                .type   = comm.type,
                .greyed = comm.quantity < 100 } );
            inner_row( std::move( spread ) );
          }
          text_row_inner( format( "{}", punit.orders_name ),
                          /*highlight=*/true );
          break;
        }
      }
    }
  }

  // Extra debugging.
  blank_line();
  text_row( format( "Royal Money: {}{}", player.royal_money,
                    config_text.special_chars.currency ) );
  text_row( format( "Zoom: {}", ss.land_view.viewport.zoom ) );
  return layout;
}

[[nodiscard]] static rect panel_render_entity_plan(
    SSConst const& ss, IVisibility const& viz,
    rr::ITextometer const& textometer, PanelRenderPlan& plan,
    point const p, PanelEntity const& entity,
    int const panel_width ) {
  static rr::TextLayout const kTextLayout{
    .monospace    = false,
    .spacing      = nothing,
    .line_spacing = nothing,
  };
  auto const add = [&]( auto& bucket, auto&& item,
                        size const offset = {} ) {
    bucket.emplace_back( std::move( item ), p + offset );
  };
  rect r;
  r.origin = p;
  SWITCH( entity ) {
    CASE( text ) {
      r.size = textometer.dimensions_for_line( kTextLayout,
                                               text.value );
      add( plan.texts,
           PanelTextRenderPlan{
             .text  = text.value,
             .color = config_ui.dialog_text.normal } );
      break;
    }
    CASE( text_highlight ) {
      r.size = textometer.dimensions_for_line(
          kTextLayout, text_highlight.value );
      add( plan.texts,
           PanelTextRenderPlan{
             .text  = text_highlight.value,
             .color = config_ui.dialog_text.highlighted } );
      break;
    }
    CASE( euro_unit ) {
      Unit const& unit = ss.units.unit_for( euro_unit.unit_id );
      r.size           = { .w = 32, .h = 32 };
      add( plan.euro_units,
           PanelUnitRenderPlan{
             .unit_id   = euro_unit.unit_id,
             .flag_info = euro_unit_flag_render_info(
                 unit, viz.player(),
                 UnitFlagOptions{
                   .flag_count =
                       euro_unit.flag_stacked
                           ? e_flag_count::multiple
                           : e_flag_count::single } ) } );
      break;
    }
    CASE( brave ) {
      NativeUnit const& native_unit =
          ss.units.unit_for( brave.native_unit_id );
      r.size = { .w = 32, .h = 32 };
      add( plan.braves,
           PanelNativeUnitRenderPlan{
             .native_unit_id = brave.native_unit_id,
             .flag_info      = native_unit_flag_render_info(
                 ss, native_unit,
                 UnitFlagOptions{
                        .flag_count =
                       brave.flag_stacked
                                ? e_flag_count::multiple
                                : e_flag_count::single } ) } );
      break;
    }
    CASE( commodity_spread ) {
      vector<TileWithOptions> tiles;
      tiles.reserve( commodity_spread.comms.size() );
      for( auto const& [type, greyed] : commodity_spread.comms )
        tiles.push_back( TileWithOptions{
          .tile   = tile_for_commodity_16( type ),
          .greyed = greyed } );
      InhomogeneousTileSpreadConfig const spread_config{
        .tiles      = std::move( tiles ),
        .options    = { .bounds = ( panel_width - p.x ) },
        .sort_tiles = true };
      TileSpreadRenderPlan spread_plan =
          build_inhomogeneous_tile_spread( textometer,
                                           spread_config );
      r.size = spread_plan.bounds.size;
      add( plan.spreads, std::move( spread_plan ) );
      break;
    }
    CASE( icon_colony ) {
      UNWRAP_CHECK_T( Colony const& colony,
                      viz.colony_at( icon_colony.tile ) );
      e_tile const sprite_tile =
          houses_tile_for_colony( colony );
      rect const trimmed = trimmed_area_for( sprite_tile );
      r.size             = trimmed.size;
      size const offset = -trimmed.origin.distance_from_origin();
      add( plan.colonies,
           PanelColonyPlan{
             .tile = icon_colony.tile,
             .options =
                 ColonyRenderOptions{ .render_name       = false,
                                      .render_population = false,
                                      .render_flag = true } },
           offset );
      break;
    }
    CASE( icon_dwelling ) {
      UNWRAP_CHECK_T( Dwelling const& dwelling,
                      viz.dwelling_at( icon_dwelling.tile ) );
      e_tile const sprite_tile =
          tile_for_dwelling( ss, dwelling );
      rect const trimmed = trimmed_area_for( sprite_tile );
      r.size             = trimmed.size;
      size const offset = -trimmed.origin.distance_from_origin();
      add( plan.dwellings, icon_dwelling.tile, offset );
      break;
    }
  }
  return r;
}

[[nodiscard]] static rect panel_render_column_plan(
    SSConst const& ss, IVisibility const& viz,
    rr::ITextometer const& textometer, PanelRenderPlan& plan,
    point const p, vector<PanelEntity> const& column,
    int const panel_width ) {
  int constexpr kRowSpacing = 1;
  point cur                 = p;
  rect total{ .origin = cur };
  for( PanelEntity const& row_entity : column ) {
    rect const r =
        panel_render_entity_plan( ss, viz, textometer, plan, cur,
                                  row_entity, panel_width );
    total = total.uni0n( r );
    cur.y += r.size.h;
    cur.y += kRowSpacing;
  }
  return total;
}

[[nodiscard]] static rect panel_render_row_plan(
    SSConst const& ss, IVisibility const& viz,
    rr::ITextometer const& textometer, PanelRenderPlan& plan,
    point const p, PanelRow const& row, int const panel_width ) {
  int constexpr kColSpacing = 1;
  point cur                 = p;
  rect total{ .origin = cur };
  for( auto const& column : row.columns ) {
    rect const r = panel_render_column_plan(
        ss, viz, textometer, plan, cur, column, panel_width );
    total = total.uni0n( r );
    cur.x += r.size.w;
    cur.x += kColSpacing;
  }
  return total;
}

PanelRenderPlan panel_render_plan(
    SSConst const& ss, IVisibility const& viz,
    rr::ITextometer const& textometer, PanelLayout const& layout,
    int const panel_width ) {
  PanelRenderPlan plan;
  int constexpr kRowSpacing = 1;
  point cur;
  rect total{ .origin = cur };
  for( PanelRow const& row : layout.rows ) {
    rect const r = panel_render_row_plan(
        ss, viz, textometer, plan, cur, row, panel_width );
    total = total.uni0n( r );
    cur.y += r.size.h;
    cur.y += kRowSpacing;
  }
  plan.area = total;
  return plan;
}

void render_panel_layout( rr::Renderer& renderer,
                          SSConst const& ss, point const where,
                          IVisibility const& viz,
                          PanelRenderPlan const& plan ) {
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation1,
      where.distance_from_origin().to_double() );

  for( auto const& [o, p] : plan.texts )
    renderer.typer( p, o.color ).write( o.text );

  for( auto const& [o, p] : plan.spreads )
    draw_rendered_icon_spread( renderer, p, o );

  for( auto const& [o, p] : plan.euro_units )
    render_unit( renderer, p, ss.units.unit_for( o.unit_id ),
                 UnitRenderOptions{ .flag = o.flag_info } );

  for( auto const& [o, p] : plan.braves )
    render_native_unit(
        renderer, p, ss.units.unit_for( o.native_unit_id ),
        UnitRenderOptions{ .flag = o.flag_info } );

  for( auto const& [o, p] : plan.colonies ) {
    UNWRAP_CHECK_T( Colony const& colony,
                    viz.colony_at( o.tile ) );
    render_colony( renderer, p, viz, colony.location, ss, colony,
                   o.options );
  }

  for( auto const& [o, p] : plan.dwellings ) {
    UNWRAP_CHECK_T( Dwelling const& dwelling,
                    viz.dwelling_at( o ) );
    render_dwelling( renderer, p, viz, o, ss, dwelling );
  }
}

} // namespace rn
