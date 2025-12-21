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
#include "macros.hpp"
#include "on-map.hpp"
#include "ts.hpp"
#include "unit-ownership.hpp"

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
#include "luapp/ext-refl.hpp"
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
#include "base/logger.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::function_ref;
using ::gfx::point;

// If the unit is working in the colony then this will return it;
// however it will not return a ColonyId if the unit simply occu-
// pies the same square as the colony.
maybe<ColonyId> colony_for_unit_who_is_worker(
    UnitsState const& units_state, UnitId id ) {
  maybe<ColonyId> res;
  if( auto const colony_state =
          units_state.ownership_of( id )
              .get_if<UnitOwnership::colony>();
      colony_state.has_value() ) {
    return colony_state->id;
  }
  return res;
}

} // namespace

/****************************************************************
** Units
*****************************************************************/
maybe<e_unit_activity> current_activity_for_unit(
    UnitsState const& units_state,
    ColoniesState const& colonies_state, UnitId id ) {
  UnitOwnership const& ownership =
      units_state.ownership_of( id );
  switch( ownership.to_enum() ) {
    case UnitOwnership::e::colony: {
      auto& o = ownership.get<UnitOwnership::colony>();
      ColonyId colony_id = o.id;
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

UnitId create_free_unit( UnitsState& units_state,
                         Player const& player,
                         UnitComposition const& comp ) {
  return units_state.add_unit(
      create_unregistered_unit( player, comp ) );
}

Unit create_unregistered_unit( Player const& player,
                               UnitComposition const& comp ) {
  wrapped::Unit refl_unit{
    .id          = UnitId{ 0 }, // will be set later.
    .composition = std::move( comp ),
    .orders      = unit_orders::none{},
    .cargo = CargoHold( unit_attr( comp.type() ).cargo_slots ),
    .player_type = player.type,
    .mv_pts      = movement_points( player, comp.type() ),
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
    SS& ss, IMapUpdater& map_updater, Player const& player,
    UnitComposition const& comp, gfx::point const coord ) {
  UnitId const id =
      create_free_unit( ss.units, player, std::move( comp ) );
  UnitOwnershipChanger( ss, id ).change_to_map_non_interactive(
      map_updater, Coord::from_gfx( coord ) );
  return id;
}

NativeUnitId create_unit_on_map_non_interactive(
    SS& ss, e_native_unit_type type, gfx::point const coord,
    DwellingId dwelling_id ) {
  NativeUnitId const native_unit_id = ss.units.add_unit_on_map(
      create_unregistered_unit( type ), Coord::from_gfx( coord ),
      dwelling_id );
  // This performs few actions that are needed when a unit moves
  // on the map (which also must be done when a unit is created
  // on the map).
  UnitOnMapMover::native_unit_to_map_non_interactive(
      ss, native_unit_id, Coord::from_gfx( coord ) );
  return native_unit_id;
}

wait<maybe<UnitId>> create_unit_on_map(
    SS& ss, TS& ts, IRand& rand, Player& player,
    UnitComposition const& comp, Coord coord ) {
  UnitId id =
      create_free_unit( ss.units, player, std::move( comp ) );
  maybe<UnitDeleted> const deleted =
      co_await UnitOwnershipChanger( ss, id ).change_to_map(
          ts, rand, coord );
  if( deleted.has_value() ) co_return nothing;
  co_return id;
}

/****************************************************************
** Type Change / Replacement.
*****************************************************************/
void change_unit_type( SS& ss, TS& ts, Unit& unit,
                       UnitComposition const& new_comp ) {
  Player const& player =
      player_for_player_or_die( ss.players, unit.player_type() );
  unit.change_type( player, new_comp );
  // In order to make sure that fog of war is updated to reflect
  // the new type (i.e. maybe its sighting radius is different),
  // we will replace it on the map.
  UnitOwnershipChanger( ss, unit.id() )
      .reinstate_on_map_if_on_map( ts.map_updater() );
}

void change_unit_player( SS& ss, TS& ts, Unit& unit,
                         e_player new_player ) {
  if( unit.player_type() == new_player ) return;
  unit.change_player( ss.units, new_player );
  // In order to make sure that fog of war is updated to reflect
  // the new type (i.e. maybe its sighting radius is different),
  // we will replace it on the map.
  UnitOwnershipChanger( ss, unit.id() )
      .reinstate_on_map_if_on_map( ts.map_updater() );
}

void change_unit_player_and_move( SS& ss, TS& ts, Unit& unit,
                                  e_player new_player,
                                  Coord target ) {
  unit.change_player( ss.units, new_player );
  UnitOwnershipChanger( ss, unit.id() )
      .change_to_map_non_interactive( ts.map_updater(), target );
}

/****************************************************************
** Map Ownership
*****************************************************************/
vector<UnitId> euro_units_from_coord(
    UnitsState const& units_state, point const tile ) {
  vector<UnitId> res;
  for( GenericUnitId const id :
       units_state.from_coord( tile ) ) {
    if( units_state.unit_kind( id ) != e_unit_kind::euro )
      continue;
    UnitId const unit_id = units_state.check_euro_unit( id );
    res.push_back( unit_id );
  }
  sort( res.begin(), res.end() ); // make it deterministic.
  return res;
}

vector<UnitId> euro_units_from_coord_recursive(
    UnitsState const& units_state, point const tile ) {
  vector<UnitId> res;
  for( GenericUnitId id : units_state.from_coord( tile ) ) {
    if( units_state.unit_kind( id ) != e_unit_kind::euro )
      continue;
    UnitId const unit_id = units_state.check_euro_unit( id );
    res.push_back( unit_id );
    auto held_units = units_state.unit_for( unit_id )
                          .cargo()
                          .items_of_type<Cargo::unit>();
    for( auto held : held_units ) res.push_back( held.id );
  }
  sort( res.begin(), res.end() ); // make it deterministic.
  return res;
}

vector<UnitId> euro_units_from_coord_recursive(
    UnitsState const& units_state, e_player const player_type,
    point const tile ) {
  auto res =
      euro_units_from_coord_recursive( units_state, tile );
  erase_if( res, [&]( UnitId const id ) {
    return units_state.unit_for( id ).player_type() !=
           player_type;
  } );
  return res;
}

std::vector<GenericUnitId> units_from_coord_recursive(
    UnitsState const& units, point const tile ) {
  vector<GenericUnitId> res;
  for( GenericUnitId const id : units.from_coord( tile ) ) {
    switch( units.unit_kind( id ) ) {
      case e_unit_kind::euro: {
        UnitId const unit_id = units.check_euro_unit( id );
        res.push_back( unit_id );
        auto held_units = units.unit_for( unit_id )
                              .cargo()
                              .items_of_type<Cargo::unit>();
        for( auto const held : held_units )
          res.push_back( held.id );
        break;
      }
      case e_unit_kind::native:
        res.push_back( id );
        break;
    }
  }
  sort( res.begin(), res.end() ); // make it deterministic.
  return res;
}

point coord_for_unit_indirect_or_die(
    UnitsState const& units_state, GenericUnitId id ) {
  UNWRAP_CHECK( res,
                coord_for_unit_indirect( units_state, id ) );
  return res;
}

maybe<point> coord_for_unit_indirect( UnitsState const& units,
                                      GenericUnitId id ) {
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
                              UnitId id ) {
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
                               UnitId id ) {
  return units_state.maybe_holder_of( id );
}

vector<UnitId> offboard_units_on_ship( SS& ss, TS& ts,
                                       Unit& ship ) {
  CHECK( ship.desc().ship );
  Coord const tile = ss.units.coord_for( ship.id() );
  UNWRAP_CHECK( cargo_units, ship.units_in_cargo() );
  for( UnitId const held_id : cargo_units ) {
    // We can use the non-interactive version here because there
    // is already a unit of the same player on the same square
    // (i.e. the ship).
    UnitOwnershipChanger( ss, held_id )
        .change_to_map_non_interactive( ts.map_updater(), tile );
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
    Unit& holder = ss.units.unit_for( holder_id );
    vector<UnitId> const removed =
        offboard_units_on_ship( ss, ts, holder );
    res.insert( res.end(), removed.begin(), removed.end() );
  }
  return res;
}

/****************************************************************
** Destruction.
*****************************************************************/
// This will safely destroy multiple units, accounting for e.g.
// the fact that one unit may be in the cargo of another unit in
// the list.
void destroy_units( SS& ss, std::vector<UnitId> const& units ) {
  for( UnitId const unit_id : units )
    if( ss.units.exists( unit_id ) )
      UnitOwnershipChanger( ss, unit_id ).destroy();
}

/****************************************************************
** Native-specific
*****************************************************************/
Tribe const& tribe_for_unit( SSConst const& ss,
                             NativeUnit const& native_unit ) {
  NativeUnitOwnership const& ownership =
      ss.units.ownership_of( native_unit.id );
  return ss.natives.tribe_for( ownership.dwelling_id );
}

Tribe& tribe_for_unit( SS& ss, NativeUnit const& native_unit ) {
  NativeUnitOwnership const& ownership =
      as_const( ss.units ).ownership_of( native_unit.id );
  return ss.natives.tribe_for( ownership.dwelling_id );
}

e_tribe tribe_type_for_unit( SSConst const& ss,
                             NativeUnit const& native_unit ) {
  NativeUnitOwnership const& ownership =
      ss.units.ownership_of( native_unit.id );
  return ss.natives.tribe_type_for( ownership.dwelling_id );
}

vector<NativeUnitId> units_for_tribe_unordered(
    SSConst const& ss, e_tribe target_tribe_type ) {
  unordered_set<DwellingId> const& dwellings =
      ss.natives.dwellings_for_tribe( target_tribe_type );
  vector<NativeUnitId> units;
  // In the vast majority of cases we will have at most one brave
  // per dwelling, so this should be fine. Sometimes we create a
  // phantom brave to act as the target of attacks on dwellings,
  // so add two just in case we call in that case, though we
  // probably won't.
  units.reserve( dwellings.size() + 2 );
  for( DwellingId const dwelling_id : dwellings ) {
    unordered_set<NativeUnitId> const& braves =
        ss.units.braves_for_dwelling( dwelling_id );
    for( NativeUnitId const native_unit_id : braves )
      units.push_back( native_unit_id );
  }
  return units;
}

set<NativeUnitId> units_for_tribe_ordered(
    SSConst const& ss, e_tribe target_tribe_type ) {
  unordered_set<DwellingId> const& dwellings =
      ss.natives.dwellings_for_tribe( target_tribe_type );
  set<NativeUnitId> units;
  for( DwellingId const dwelling_id : dwellings ) {
    unordered_set<NativeUnitId> const& braves =
        ss.units.braves_for_dwelling( dwelling_id );
    for( NativeUnitId const native_unit_id : braves )
      units.insert( native_unit_id );
  }
  return units;
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

LUA_FN( create_unit_on_map, Unit&, e_player const player_type,
        UnitComposition& comp, Coord const& coord ) {
  SS& ss = st["SS"].as<SS&>();
  IMapUpdater& map_updater =
      st["IMapUpdater"].as<IMapUpdater&>();
  maybe<Player const&> player = ss.players.players[player_type];
  LUA_CHECK( st, player.has_value(),
             "player for player {} does not exist.", player );
  UnitId id = create_unit_on_map_non_interactive(
      ss, map_updater, *player, comp, coord );
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

LUA_FN( create_unit_in_cargo, Unit&, e_player const player_type,
        UnitComposition& comp, UnitId holder ) {
  SS& ss                      = st["SS"].as<SS&>();
  maybe<Player const&> player = ss.players.players[player_type];
  LUA_CHECK( st, player.has_value(),
             "player for player {} does not exist.", player );
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
