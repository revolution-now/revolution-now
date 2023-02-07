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
#include "imap-updater.hpp"
#include "land-production.hpp"
#include "logger.hpp"
#include "macros.hpp"
#include "map-updater-lua.hpp"
#include "on-map.hpp"
#include "ts.hpp"
#include "variant.hpp"

// config
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/dwelling.rds.hpp"
#include "ss/native-unit.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/cdr.hpp"
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
  UnitOwnership_t const& ownership =
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
      .orders      = e_unit_orders::none,
      .cargo = CargoHold( unit_attr( comp.type() ).cargo_slots ),
      .nation = player.nation,
      .mv_pts = movement_points( player, comp.type() ),
  };
  return Unit( std::move( refl_unit ) );
}

NativeUnit create_unregistered_unit( e_native_unit_type type ) {
  return NativeUnit{
      .id   = NativeUnitId{ 0 }, // will be set later.
      .type = type };
}

UnitId create_unit_on_map_non_interactive( SS& ss, TS& ts,
                                           Player const& player,
                                           UnitComposition comp,
                                           Coord coord ) {
  UnitId id =
      create_free_unit( ss.units, player, std::move( comp ) );
  unit_to_map_square_non_interactive( ss, ts, id, coord );
  return id;
}

NativeUnitId create_unit_on_map_non_interactive(
    SS& ss, e_native_unit_type type, Coord coord,
    DwellingId dwelling_id ) {
  NativeUnitId const id = create_free_unit( ss, type );
  unit_to_map_square_non_interactive( ss, id, coord,
                                      dwelling_id );
  return id;
}

wait<maybe<UnitId>> create_unit_on_map( SS& ss, TS& ts,
                                        Player&         player,
                                        UnitComposition comp,
                                        Coord           coord ) {
  UnitId id =
      create_free_unit( ss.units, player, std::move( comp ) );
  maybe<UnitDeleted> unit_deleted =
      co_await unit_to_map_square( ss, ts, id, coord );
  if( unit_deleted.has_value() ) co_return nothing;
  co_return id;
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
      UnitOwnership_t const& ownership =
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
e_tribe tribe_for_unit( SSConst const&    ss,
                        NativeUnit const& native_unit ) {
  NativeUnitOwnership_t const& ownership =
      ss.units.ownership_of( native_unit.id );
  UNWRAP_CHECK( world,
                ownership.get_if<NativeUnitOwnership::world>() );
  return ss.natives
      .tribe_for(
          ss.natives.dwelling_for( world.dwelling_id ).id )
      .type;
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
  lg.info( "created a {} on square {}.",
           unit_attr( comp.type() ).name, coord );
  return ss.units.unit_for( id );
}

LUA_FN( create_native_unit_on_map, NativeUnit&,
        DwellingId dwelling_id, e_native_unit_type type,
        Coord coord ) {
  SS& ss = st["SS"].as<SS&>();

  NativeUnitId const id = create_unit_on_map_non_interactive(
      ss, type, coord, dwelling_id );
  lg.info( "created a {} on square {}.", type, coord );
  return ss.units.unit_for( id );
}

LUA_FN( add_unit_to_cargo, void, UnitId held, UnitId holder ) {
  SS& ss = st["SS"].as<SS&>();
  lg.info( "adding unit {} to cargo of unit {}.",
           debug_string( ss.units, held ),
           debug_string( ss.units, holder ) );
  ss.units.change_to_cargo_somewhere( holder, held );
}

LUA_FN( create_unit_in_cargo, Unit&, e_nation nation,
        UnitComposition& comp, UnitId holder ) {
  SS&                  ss     = st["SS"].as<SS&>();
  maybe<Player const&> player = ss.players.players[nation];
  LUA_CHECK( st, player.has_value(),
             "player for nation {} does not exist.", nation );
  UnitId unit_id = create_free_unit( ss.units, *player, comp );
  lg.info( "created unit {}.", debug_string( ss.units, unit_id ),
           debug_string( ss.units, holder ) );
  ss.units.change_to_cargo_somewhere( holder, unit_id );
  return ss.units.unit_for( unit_id );
}

} // namespace

} // namespace rn
