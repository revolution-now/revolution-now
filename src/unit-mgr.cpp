/****************************************************************
**unit-mgr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-08.
*
* Description: Handles creation, destruction, and ownership of
*              units.
*
*****************************************************************/
#include "unit-mgr.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-buildings.hpp"
#include "colony.hpp"
#include "error.hpp"
#include "harbor-units.hpp"
#include "imap-updater.hpp"
#include "land-production.hpp"
#include "logger.hpp"
#include "macros.hpp"
#include "map-updater-lua.hpp"
#include "on-map.hpp"
#include "ts.hpp"
#include "variant.hpp"

// config
#include "config/natives.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/dwelling.rds.hpp"
#include "ss/native-unit.hpp"
#include "ss/natives.hpp"
#include "ss/player.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/function-ref.hpp"
#include "base/keyval.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::function_ref;

// If the unit is working in the colony then this will return it;
// however it will not return a ColonyId if the unit simply occu-
// pies the same square as the colony.
maybe<ColonyId> colony_for_unit_who_is_worker(
    UnitsState const& units_state, UnitId id ) {
  maybe<ColonyId> res;
  if_get( units_state.ownership_of( id ), UnitOwnership::colony,
          colony_state ) {
    return colony_state.id;
  }
  return res;
}

} // namespace

/****************************************************************
** Units
*****************************************************************/
maybe<e_unit_activity> current_activity_for_unit(
    UnitsState const&    units_state,
    ColoniesState const& colonies_state, UnitId id ) {
  UnitOwnership const& ownership =
      units_state.ownership_of( id );
  switch( ownership.to_enum() ) {
    case UnitOwnership::e::colony: {
      auto&         o = ownership.get<UnitOwnership::colony>();
      ColonyId      colony_id = o.id;
      Colony const& colony =
          colonies_state.colony_for( colony_id );
      // First check outdoor jobs.
      for( e_direction d : refl::enum_values<e_direction> ) {
        maybe<OutdoorUnit const&> outdoor_unit =
            colony.outdoor_jobs[d];
        if( outdoor_unit.has_value() &&
            outdoor_unit->unit_id == id )
          return activity_for_outdoor_job( outdoor_unit->job );
      }
      // Next check indoor jobs.
      for( e_indoor_job job : refl::enum_values<e_indoor_job> ) {
        vector<UnitId> const& units = colony.indoor_jobs[job];
        if( find( units.begin(), units.end(), id ) !=
            units.end() )
          return activity_for_indoor_job( job );
      }
      return nothing;
    }
    case UnitOwnership::e::dwelling:
      // For this one the unit is in a dwelling, meaning that it
      // is a missionary. Since only a missionary unit can be put
      // in a dwelling, the type_activity will suffice.
      break;
    case UnitOwnership::e::cargo:
    case UnitOwnership::e::free:
    case UnitOwnership::e::harbor:
    case UnitOwnership::e::world:
      break;
  }

  return units_state.unit_for( id ).desc().type_activity;
}

string debug_string( UnitsState const& units_state, UnitId id ) {
  return debug_string( units_state.unit_for( id ) );
}

UnitId create_free_unit( UnitsState&     units_state,
                         Player const&   player,
                         UnitComposition comp ) {
  return units_state.add_unit(
      create_unregistered_unit( player, comp ) );
}

NativeUnitId create_free_unit( SS&                ss,
                               e_native_unit_type type ) {
  return ss.units.add_unit( create_unregistered_unit( type ) );
}

Unit create_unregistered_unit( Player const&   player,
                               UnitComposition comp ) {
  wrapped::Unit refl_unit{
      .id          = UnitId{ 0 }, // will be set later.
      .composition = std::move( comp ),
      .orders      = unit_orders::none{},
      .cargo = CargoHold( unit_attr( comp.type() ).cargo_slots ),
      .nation = player.nation,
      .mv_pts = movement_points( player, comp.type() ),
  };
  return Unit( std::move( refl_unit ) );
}

NativeUnit create_unregistered_unit( e_native_unit_type type ) {
  return NativeUnit{
      .id              = NativeUnitId{ 0 }, // will be set later.
      .type            = type,
      .movement_points = unit_attr( type ).movement_points };
}

UnitId create_unit_on_map_non_interactive( SS& ss, TS& ts,
                                           Player const& player,
                                           UnitComposition comp,
                                           Coord coord ) {
  UnitId id =
      create_free_unit( ss.units, player, std::move( comp ) );
  unit_ownership_change_non_interactive(
      ss, id,
      EuroUnitOwnershipChangeTo::world{ .ts     = &ts,
                                        .target = coord } );
  return id;
}

NativeUnitId create_unit_on_map_non_interactive(
    SS& ss, e_native_unit_type type, Coord coord,
    DwellingId dwelling_id ) {
  NativeUnitId const id = create_free_unit( ss, type );
  UnitOnMapMover::native_unit_to_map_non_interactive(
      ss, id, coord, dwelling_id );
  return id;
}

wait<maybe<UnitId>> create_unit_on_map( SS& ss, TS& ts,
                                        Player&         player,
                                        UnitComposition comp,
                                        Coord           coord ) {
  UnitId id =
      create_free_unit( ss.units, player, std::move( comp ) );
  maybe<UnitDeleted> const deleted =
      co_await unit_ownership_change(
          ss, id,
          EuroUnitOwnershipChangeTo::world{ .ts     = &ts,
                                            .target = coord } );
  if( deleted.has_value() ) co_return nothing;
  co_return id;
}

/****************************************************************
** Type Change / Replacement.
*****************************************************************/
void change_unit_type( SS& ss, TS& ts, Unit& unit,
                       UnitComposition new_comp ) {
  Player const& player =
      player_for_nation_or_die( ss.players, unit.nation() );
  unit.change_type( player, new_comp );
  UnitOwnership const& ownership =
      as_const( ss.units ).ownership_of( unit.id() );
  switch( ownership.to_enum() ) {
    case UnitOwnership::e::world: {
      auto& o = ownership.get<UnitOwnership::world>();
      // In order to make sure that fog of war is updated to re-
      // flect the new type (i.e. maybe its sighting radius is
      // different), we will replace it on the map.
      unit_ownership_change_non_interactive(
          ss, unit.id(),
          EuroUnitOwnershipChangeTo::world{
              .ts = &ts, .target = o.coord } );
      break;
    }
    default:
      break;
  }
}

void change_unit_nation( SS& ss, TS& ts, Unit& unit,
                         e_nation new_nation ) {
  unit.change_nation( ss.units, new_nation );
  UnitOwnership const& ownership =
      as_const( ss.units ).ownership_of( unit.id() );
  switch( ownership.to_enum() ) {
    case UnitOwnership::e::world: {
      auto& o = ownership.get<UnitOwnership::world>();
      // In order to make sure that fog of war is updated to re-
      // flect the new type (i.e. maybe its sighting radius is
      // different), we will replace it on the map.
      unit_ownership_change_non_interactive(
          ss, unit.id(),
          EuroUnitOwnershipChangeTo::world{
              .ts = &ts, .target = o.coord } );
      break;
    }
    default:
      break;
  }
}

void change_unit_nation_and_move( SS& ss, TS& ts, Unit& unit,
                                  e_nation new_nation,
                                  Coord    target ) {
  unit.change_nation( ss.units, new_nation );
  unit_ownership_change_non_interactive(
      ss, unit.id(),
      EuroUnitOwnershipChangeTo::world{ .ts     = &ts,
                                        .target = target } );
}

/****************************************************************
** Map Ownership
*****************************************************************/
vector<UnitId> euro_units_from_coord_recursive(
    UnitsState const& units_state, Coord coord ) {
  vector<UnitId> res;
  for( GenericUnitId id : units_state.from_coord( coord ) ) {
    if( units_state.unit_kind( id ) != e_unit_kind::euro )
      continue;
    UnitId const unit_id = units_state.check_euro_unit( id );
    res.push_back( unit_id );
    auto held_units = units_state.unit_for( unit_id )
                          .cargo()
                          .items_of_type<Cargo::unit>();
    for( auto held : held_units ) res.push_back( held.id );
  }
  return res;
}

ND Coord coord_for_unit_indirect_or_die(
    UnitsState const& units_state, GenericUnitId id ) {
  UNWRAP_CHECK( res,
                coord_for_unit_indirect( units_state, id ) );
  return res;
}

maybe<Coord> coord_for_unit_indirect( UnitsState const& units,
                                      GenericUnitId     id ) {
  switch( units.unit_kind( id ) ) {
    case e_unit_kind::euro: {
      CHECK( units.exists( id ) );
      UnitOwnership const& ownership =
          units.ownership_of( units.check_euro_unit( id ) );
      switch( ownership.to_enum() ) {
        case UnitOwnership::e::world: {
          auto& [coord] = ownership.get<UnitOwnership::world>();
          return coord;
        }
        case UnitOwnership::e::cargo: {
          auto& [holder] = ownership.get<UnitOwnership::cargo>();
          return coord_for_unit_indirect( units, holder );
        }
        case UnitOwnership::e::free:
        case UnitOwnership::e::harbor:
        case UnitOwnership::e::colony:
        case UnitOwnership::e::dwelling: //
          return nothing;
      };
      SHOULD_NOT_BE_HERE;
    }
    case e_unit_kind::native: {
      return units.maybe_coord_for(
          units.check_native_unit( id ) );
    }
  }
}

bool is_unit_on_map_indirect( UnitsState const& units_state,
                              UnitId            id ) {
  return coord_for_unit_indirect( units_state, id ).has_value();
}

bool is_unit_on_map( UnitsState const& units_state, UnitId id ) {
  return units_state.ownership_of( id )
      .holds<UnitOwnership::world>();
}

/****************************************************************
** Cargo Ownership
*****************************************************************/
// If the unit is being held as cargo then it will return the id
// of the unit that is holding it; nothing otherwise.
maybe<UnitId> is_unit_onboard( UnitsState const& units_state,
                               UnitId            id ) {
  return units_state.maybe_holder_of( id );
}

/****************************************************************
** Native-specific
*****************************************************************/
Tribe const& tribe_for_unit( SSConst const&    ss,
                             NativeUnit const& native_unit ) {
  NativeUnitOwnership const& ownership =
      ss.units.ownership_of( native_unit.id );
  UNWRAP_CHECK( world,
                ownership.get_if<NativeUnitOwnership::world>() );
  return ss.natives.tribe_for( world.dwelling_id );
}

Tribe& tribe_for_unit( SS& ss, NativeUnit const& native_unit ) {
  NativeUnitOwnership const& ownership =
      as_const( ss.units ).ownership_of( native_unit.id );
  UNWRAP_CHECK( world,
                ownership.get_if<NativeUnitOwnership::world>() );
  return ss.natives.tribe_for( world.dwelling_id );
}

e_tribe tribe_type_for_unit( SSConst const&    ss,
                             NativeUnit const& native_unit ) {
  NativeUnitOwnership const& ownership =
      ss.units.ownership_of( native_unit.id );
  UNWRAP_CHECK( world,
                ownership.get_if<NativeUnitOwnership::world>() );
  return ss.natives.tribe_type_for( world.dwelling_id );
}

/****************************************************************
** Multi
*****************************************************************/
maybe<Coord> coord_for_unit_multi_ownership( SSConst const& ss,
                                             GenericUnitId id ) {
  if( auto maybe_map = coord_for_unit_indirect( ss.units, id );
      maybe_map )
    return maybe_map;
  if( ss.units.unit_kind( id ) == e_unit_kind::euro ) {
    UnitId const unit_id = ss.units.check_euro_unit( id );
    if( auto maybe_colony =
            colony_for_unit_who_is_worker( ss.units, unit_id ) )
      return ss.colonies.colony_for( *maybe_colony ).location;
    if( maybe<DwellingId> dwelling_id =
            ss.units.maybe_dwelling_for_missionary( unit_id );
        dwelling_id.has_value() )
      return ss.natives.coord_for( *dwelling_id );
  }
  return nothing;
}

Coord coord_for_unit_multi_ownership_or_die( SSConst const& ss,
                                             GenericUnitId id ) {
  UNWRAP_CHECK( res, coord_for_unit_multi_ownership( ss, id ) );
  return res;
}

/****************************************************************
** Unit Ownership changes.
*****************************************************************/
void unit_ownership_change_non_interactive(
    SS& ss, UnitId unit_id,
    EuroUnitOwnershipChangeTo const& change_to ) {
  Unit const& unit = ss.units.unit_for( unit_id );
  Player&     player =
      player_for_nation_or_die( ss.players, unit.nation() );
  SWITCH( change_to ) {
    CASE( free ) {
      ss.units.disown_unit( unit_id );
      return;
    }
    CASE( world ) {
      TS& ts = *world.ts.get();
      UnitOnMapMover::to_map_non_interactive( ss, ts, unit_id,
                                              world.target );
      return;
    }
    CASE( colony_low_level ) {
      // NOTE: do not use this one directly since it does not
      // preserve invariants; instead use the move_unit_to_colony
      // method.
      ss.units.change_to_colony( unit_id,
                                 colony_low_level.colony_id );
      return;
    }
    CASE( cargo ) {
      ss.units.change_to_cargo_somewhere( cargo.new_holder,
                                          /*held=*/unit_id,
                                          cargo.starting_slot );
      return;
    }
    CASE( sail_to_new_world ) {
      UnitHarborMover::unit_sail_to_new_world(
          ss.terrain, ss.units, player, unit_id );
      return;
    }
    CASE( sail_to_harbor ) {
      UnitHarborMover::unit_sail_to_harbor( ss.terrain, ss.units,
                                            player, unit_id );
      return;
    }
    CASE( move_to_port ) {
      UnitHarborMover::unit_move_to_port( ss.units, player,
                                          unit_id );
      return;
    }
    CASE( dwelling ) {
      ss.units.change_to_dwelling( unit_id,
                                   dwelling.dwelling_id );
      return;
    }
  }
}

wait<maybe<UnitDeleted>> unit_ownership_change(
    SS& ss, UnitId unit_id,
    EuroUnitOwnershipChangeTo const& change_to ) {
  switch( change_to.to_enum() ) {
    using e = EuroUnitOwnershipChangeTo::e;
    case e::world: {
      auto& o =
          change_to.get<EuroUnitOwnershipChangeTo::world>();
      TS& ts = *o.ts.get();
      co_return co_await UnitOnMapMover::to_map_interactive(
          ss, ts, unit_id, o.target );
    }
    default:
      unit_ownership_change_non_interactive( ss, unit_id,
                                             change_to );
      co_return nothing;
  }
}

void destroy_unit( SS& ss, GenericUnitId id ) {
  switch( ss.units.unit_kind( id ) ) {
    case e_unit_kind::euro: {
      UnitId const unit_id = ss.units.check_euro_unit( id );
      // FIXME: the unit ownership/destruction mechanism is not
      // consistent in which unit states allow direct deletion
      // and which require cleanup, and then is also inconsistent
      // in what states it cleans up and which it does not (i.e.,
      // the below function call will clean up if the unit is
      // held as cargo, but not if the unit is a colony worker).
      CHECK( !as_const( ss.units )
                  .ownership_of( unit_id )
                  .holds<UnitOwnership::colony>(),
             "This method isn't equipped to directly destroy a "
             "unit that is in a colony; you must first call "
             "remove_unit_from_colony." );
      ss.units.destroy_unit( unit_id );
      break;
    }
    case e_unit_kind::native:
      ss.units.destroy_unit( ss.units.check_native_unit( id ) );
      break;
  }
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( create_unit_on_map, Unit&, e_nation nation,
        UnitComposition& comp, Coord const& coord ) {
  SS&                  ss     = st["SS"].as<SS&>();
  TS&                  ts     = st["TS"].as<TS&>();
  maybe<Player const&> player = ss.players.players[nation];
  LUA_CHECK( st, player.has_value(),
             "player for nation {} does not exist.", nation );
  UnitId id = create_unit_on_map_non_interactive(
      ss, ts, *player, comp, coord );
  lg.trace( "created a {} on square {}.",
            unit_attr( comp.type() ).name, coord );
  return ss.units.unit_for( id );
}

LUA_FN( create_native_unit_on_map, NativeUnit&,
        DwellingId dwelling_id, e_native_unit_type type,
        Coord coord ) {
  SS& ss = st["SS"].as<SS&>();

  NativeUnitId const id = create_unit_on_map_non_interactive(
      ss, type, coord, dwelling_id );
  lg.trace( "created a {} on square {}.", type, coord );
  return ss.units.unit_for( id );
}

LUA_FN( add_unit_to_cargo, void, UnitId held, UnitId holder ) {
  SS& ss = st["SS"].as<SS&>();
  lg.trace( "adding unit {} to cargo of unit {}.",
            debug_string( ss.units, held ),
            debug_string( ss.units, holder ) );
  unit_ownership_change_non_interactive(
      ss, held,
      EuroUnitOwnershipChangeTo::cargo{ .new_holder    = holder,
                                        .starting_slot = 0 } );
}

LUA_FN( create_unit_in_cargo, Unit&, e_nation nation,
        UnitComposition& comp, UnitId holder ) {
  SS&                  ss     = st["SS"].as<SS&>();
  maybe<Player const&> player = ss.players.players[nation];
  LUA_CHECK( st, player.has_value(),
             "player for nation {} does not exist.", nation );
  UnitId unit_id = create_free_unit( ss.units, *player, comp );
  lg.trace( "created unit {}.",
            debug_string( ss.units, unit_id ),
            debug_string( ss.units, holder ) );
  unit_ownership_change_non_interactive(
      ss, unit_id,
      EuroUnitOwnershipChangeTo::cargo{ .new_holder    = holder,
                                        .starting_slot = 0 } );
  return ss.units.unit_for( unit_id );
}

} // namespace

} // namespace rn
