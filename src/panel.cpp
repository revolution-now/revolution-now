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
#include "roles.hpp"
#include "society.hpp"
#include "unit-mgr.hpp"
#include "unit-stack.hpp"
#include "visibility.hpp"
#include "white-box.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/turn.rds.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

namespace rn {

namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::gfx::point;

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
PanelEntities entities_shown_on_panel( SSConst const& ss ) {
  PanelEntities entities;

  auto const viewer =
      player_for_role( ss, e_player_role::viewer );
  auto const viz = create_visibility_for( ss, viewer );

  auto const populate_colony = [&]( point const tile ) {
    // If the entire map is visible then there are no constraints
    // on what can be seen, regardless of player/ownership.
    if( !viewer.has_value() ) {
      auto const colony_id =
          ss.colonies.maybe_from_coord( tile );
      if( !colony_id.has_value() ) return;
      entities.city =
          PanelCity::visible_colony{ .colony_id = *colony_id };
      return;
    }
    auto const colony = viz->colony_at( tile );
    if( !colony.has_value() ) return;
    CHECK( viewer.has_value() );
    if( colony->player == *viewer ) {
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
    if( viz->dwelling_at( tile ).has_value() )
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
    if( !viewer.has_value() ) {
      show_all_units();
      return;
    }
    CHECK( viewer.has_value() );
    VisibleSociety const viz_society =
        society_on_visible_square( ss, *viz, tile );
    SWITCH( viz_society ) {
      CASE( hidden ) { return; }
      CASE( empty ) { return; }
      CASE( society ) {
        Society const& soc = society.value;
        SWITCH( soc ) {
          CASE( european ) {
            if( european.player == *viewer )
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
    entities.square = viz->square_at( tile );
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
      if( viewer.has_value() && *viewer != player.type ) break;
      entities.player = player.type;
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

} // namespace rn
