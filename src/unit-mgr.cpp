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
#include "unit-ownership.hpp"
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

UnitId create_free_unit( UnitsState&            units_state,
                         Player const&          player,
                         UnitComposition const& comp ) {
  return units_state.add_unit(
      create_unregistered_unit( player, comp ) );
}

NativeUnitId create_free_unit( SS&                ss,
                               e_native_unit_type type ) {
  return ss.units.add_unit( create_unregistered_unit( type ) );
}

Unit create_unregistered_unit( Player const&          player,
                               UnitComposition const& comp ) {
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

UnitId create_unit_on_map_non_interactive(
    SS& ss, TS& ts, Player const& player,
    UnitComposition const& comp, Coord coord ) {
  UnitId const id =
      create_free_unit( ss.units, player, std::move( comp ) );
  UnitOwnershipChanger( ss, id ).change_to_map_non_interactive(
      ts, coord );
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

wait<maybe<UnitId>> create_unit_on_map(
    SS& ss, TS& ts, Player& player, UnitComposition const& comp,
    Coord coord ) {
  UnitId id =
      create_free_unit( ss.units, player, std::move( comp ) );
  maybe<UnitDeleted> const deleted =
      co_await UnitOwnershipChanger( ss, id ).change_to_map(
          ts, coord );
  if( deleted.has_value() ) co_return nothing;
  co_return id;
}

/****************************************************************
** Type Change / Replacement.
*****************************************************************/
void change_unit_type( SS& ss, TS& ts, Unit& unit,
                       UnitComposition const& new_comp ) {
  Player const& player =
      player_for_nation_or_die( ss.players, unit.nation() );
  unit.change_type( player, new_comp );
  // In order to make sure that fog of war is updated to reflect
  // the new type (i.e. maybe its sighting radius is different),
  // we will replace it on the map.
  UnitOwnershipChanger( ss, unit.id() )
      .reinstate_on_map_if_on_map( ts );
}

void change_unit_nation( SS& ss, TS& ts, Unit& unit,
                         e_nation new_nation ) {
  unit.change_nation( ss.units, new_nation );
  // In order to make sure that fog of war is updated to reflect
  // the new type (i.e. maybe its sighting radius is different),
  // we will replace it on the map.
  UnitOwnershipChanger( ss, unit.id() )
      .reinstate_on_map_if_on_map( ts );
}

void change_unit_nation_and_move( SS& ss, TS& ts, Unit& unit,
                                  e_nation new_nation,
                                  Coord    target ) {
  unit.change_nation( ss.units, new_nation );
  UnitOwnershipChanger( ss, unit.id() )
      .change_to_map_non_interactive( ts, target );
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

vector<UnitId> offboard_units_on_ship( SS& ss, TS& ts,
                                       Unit& ship ) {
  CHECK( ship.desc().ship );
  Coord const tile = ss.units.coord_for( ship.id() );
  UNWRAP_CHECK( cargo_units, ship.units_in_cargo() );
  for( UnitId const held_id : cargo_units ) {
    // We can use the non-interactive version here because there
    // is already a unit of the same nation on the same square
    // (i.e. the ship).
    UnitOwnershipChanger( ss, held_id )
        .change_to_map_non_interactive( ts, tile );
    // Reproduce the behavior of the OG where "units on ships"
    // are really just sentried on the map.
    ss.units.unit_for( held_id ).sentry();
  }
  return cargo_units;
}

vector<UnitId> offboard_units_on_ships( SS& ss, TS& ts,
                                        Coord coord ) {
  // NOTE: offboarding units means putting them onto the map,
  // which will change the set that we are iterating over here,
  // which is why we gather the units to offboard before removing
  // them.
  vector<UnitId> ships;
  for( GenericUnitId const id : ss.units.from_coord( coord ) ) {
    if( ss.units.unit_kind( id ) != e_unit_kind::euro ) continue;
    Unit const& holder = ss.units.euro_unit_for( id );
    if( !holder.desc().ship ) continue;
    ships.push_back( holder.id() );
  }
  vector<UnitId> res;
  for( UnitId const holder_id : ships ) {
    Unit&                holder = ss.units.unit_for( holder_id );
    vector<UnitId> const removed =
        offboard_units_on_ship( ss, ts, holder );
    res.insert( res.end(), removed.begin(), removed.end() );
  }
  return res;
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
  UnitOwnershipChanger( ss, held )
      .change_to_cargo( holder, /*starting_slot=*/0 );
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
  UnitOwnershipChanger( ss, unit_id )
      .change_to_cargo( holder, /*starting_slot=*/0 );
  return ss.units.unit_for( unit_id );
}

} // namespace

} // namespace rn
