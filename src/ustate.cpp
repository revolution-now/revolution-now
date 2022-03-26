/****************************************************************
**ustate.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-08.
*
* Description: Handles creation, destruction, and ownership of
*              units.
*
*****************************************************************/
#include "ustate.hpp"

// Revolution Now
#include "colony.hpp"
#include "cstate.hpp"
#include "error.hpp"
#include "game-state.hpp"
#include "gs-units.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "macros.hpp"
#include "variant.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/cdr.hpp"
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/function-ref.hpp"
#include "base/keyval.hpp"
#include "base/lambda.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::function_ref;

} // namespace

/****************************************************************
** Units
*****************************************************************/
string debug_string( UnitId id ) {
  auto& gs_units = GameState::units();
  return debug_string( gs_units.unit_for( id ) );
}

vector<UnitId> units_all() {
  auto&          gs_units = GameState::units();
  vector<UnitId> res;
  res.reserve( gs_units.all().size() );
  for( auto const& p : gs_units.all() ) res.push_back( p.first );
  return res;
}

vector<UnitId> units_all( e_nation n ) {
  auto&          gs_units = GameState::units();
  vector<UnitId> res;
  res.reserve( gs_units.all().size() );
  for( auto const& p : gs_units.all() )
    if( n == p.second.unit.nation() ) res.push_back( p.first );
  return res;
}

bool unit_exists( UnitId id ) {
  auto& gs_units = GameState::units();
  return gs_units.all().contains( id );
}

Unit& unit_from_id( UnitId id ) {
  return GameState::units().unit_for( id );
}

// Apply a function to all units. The function may mutate the
// units.
void map_units( function_ref<void( Unit& )> func ) {
  auto& gs_units = GameState::units();
  for( auto& p : gs_units.all() )
    func( gs_units.unit_for( p.first ) );
}

void map_units( e_nation                    nation,
                function_ref<void( Unit& )> func ) {
  auto& gs_units = GameState::units();
  for( auto& p : gs_units.all() ) {
    Unit& unit = gs_units.unit_for( p.first );
    if( unit.nation() == nation ) func( unit );
  }
}

UnitId create_unit( UnitsState& units_state, e_nation nation,
                    UnitComposition comp ) {
  wrapped::Unit refl_unit{
      .id          = UnitId{ 0 }, // will be set later.
      .composition = std::move( comp ),
      .orders      = e_unit_orders::none,
      .cargo = CargoHold( unit_attr( comp.type() ).cargo_slots ),
      .nation = nation,
      .mv_pts = unit_attr( comp.type() ).movement_points,
  };
  return units_state.add_unit( Unit( std::move( refl_unit ) ) );
}

UnitId create_unit( UnitsState& units_state, e_nation nation,
                    UnitType type ) {
  return create_unit( units_state, nation,
                      UnitComposition::create( type ) );
}

/****************************************************************
** Map Ownership
*****************************************************************/
unordered_set<UnitId> const& units_from_coord( Coord const& c ) {
  return GameState::units().from_coord( c );
}

vector<UnitId> units_from_coord_recursive( Coord coord ) {
  auto&          gs_units = GameState::units();
  vector<UnitId> res;
  for( auto id : units_from_coord( coord ) ) {
    res.push_back( id );
    auto held_units = gs_units.unit_for( id )
                          .cargo()
                          .items_of_type<Cargo::unit>();
    for( auto held : held_units ) res.push_back( held.id );
  }
  return res;
}

void move_unit_from_map_to_map( UnitId id, Coord dst ) {
  auto& gs_units = GameState::units();
  CHECK( as_const( gs_units )
             .ownership_of( id )
             .holds<UnitOwnership::world>() );
  gs_units.change_to_map( id, dst );
}

vector<UnitId> units_in_rect( Rect const& rect ) {
  vector<UnitId> res;
  for( Y i = rect.y; i < rect.y + rect.h; ++i )
    for( X j = rect.x; j < rect.x + rect.w; ++j )
      for( auto id : units_from_coord( Coord{ i, j } ) )
        res.push_back( id );
  return res;
}

vector<UnitId> surrounding_units( Coord const& coord ) {
  vector<UnitId> res;
  for( e_direction d : refl::enum_values<e_direction> ) {
    if( d == e_direction::c ) continue;
    for( auto id : units_from_coord( coord.moved( d ) ) )
      res.push_back( id );
  }
  return res;
}

maybe<Coord> coord_for_unit( UnitId id ) {
  return GameState::units().maybe_coord_for( id );
}

Coord coord_for_unit_indirect_or_die( UnitId id ) {
  UNWRAP_CHECK( res, coord_for_unit_indirect( id ) );
  return res;
}

// If this function makes recursive calls it should always call
// the _safe variant since this function should not throw.
maybe<Coord> coord_for_unit_indirect( UnitId id ) {
  auto const& gs_units = GameState::units();
  CHECK( unit_exists( id ) );
  UnitOwnership_t const& ownership = gs_units.ownership_of( id );
  switch( ownership.to_enum() ) {
    case UnitOwnership::e::world: {
      auto& [coord] = ownership.get<UnitOwnership::world>();
      return coord;
    }
    case UnitOwnership::e::cargo: {
      auto& [holder] = ownership.get<UnitOwnership::cargo>();
      return coord_for_unit_indirect( holder );
    }
    case UnitOwnership::e::free:
    case UnitOwnership::e::old_world:
    case UnitOwnership::e::colony: //
      return nothing;
  };
}

bool is_unit_on_map_indirect( UnitId id ) {
  return coord_for_unit_indirect( id ).has_value();
}

bool is_unit_on_map( UnitId id ) {
  auto const& gs_units = GameState::units();
  return gs_units.ownership_of( id )
      .holds<UnitOwnership::world>();
}

/****************************************************************
** Colony Ownership
*****************************************************************/
unordered_set<UnitId> units_at_or_in_colony( ColonyId id ) {
  auto& gs_units = GameState::units();
  CHECK( colony_exists( id ) );
  unordered_set<UnitId> all = gs_units.from_colony( id );
  Coord colony_loc          = colony_from_id( id ).location();
  for( UnitId map_id : units_from_coord( colony_loc ) )
    all.insert( map_id );
  return all;
}

maybe<ColonyId> colony_for_unit_who_is_worker( UnitId id ) {
  auto const&     gs_units = GameState::units();
  maybe<ColonyId> res;
  if_get( gs_units.ownership_of( id ), UnitOwnership::colony,
          colony_state ) {
    return colony_state.id;
  }
  return res;
}

bool is_unit_in_colony( UnitId id ) {
  auto const& gs_units = GameState::units();
  return gs_units.ownership_of( id )
      .holds<UnitOwnership::colony>();
}

/****************************************************************
** Cargo Ownership
*****************************************************************/
// If the unit is being held as cargo then it will return the id
// of the unit that is holding it; nothing otherwise.
maybe<UnitId> is_unit_onboard( UnitId id ) {
  auto& gs_units = GameState::units();
  return gs_units.maybe_holder_of( id );
}

/****************************************************************
** Old World View Ownership
*****************************************************************/
valid_or<string> UnitOldWorldViewState::outbound::validate()
    const {
  RETURN_IF_FALSE( percent >= 0.0,
                   "ship outbound percentage must be between 0 "
                   "and 1 inclusive, but is {}.",
                   percent );
  RETURN_IF_FALSE( percent <= 1.0,
                   "ship outbound percentage must be between 0 "
                   "and 1 inclusive, but is {}.",
                   percent );
  return valid;
}

valid_or<string> UnitOldWorldViewState::inbound::validate()
    const {
  RETURN_IF_FALSE( percent >= 0.0,
                   "ship outbound percentage must be between 0 "
                   "and 1 inclusive, but is {}.",
                   percent );
  RETURN_IF_FALSE( percent <= 1.0,
                   "ship outbound percentage must be between 0 "
                   "and 1 inclusive, but is {}.",
                   percent );
  return valid;
}

valid_or<string> UnitOldWorldViewState::in_port::validate()
    const {
  return valid;
}

valid_or<generic_err> check_old_world_state_invariants(
    UnitOldWorldViewState_t const& info ) {
  valid_or<string> res = std::visit( L( _.validate() ), info );
  if( !res ) return GENERIC_ERROR( "{}", res.error() );
  return base::valid;
}

maybe<UnitOldWorldViewState_t&> unit_old_world_view_info(
    UnitId id ) {
  auto& gs_units = GameState::units();
  return gs_units.maybe_old_world_view_state_of( id );
}

vector<UnitId> units_in_old_world_view() {
  vector<UnitId> res;
  auto&          gs_units = GameState::units();
  for( auto const& [id, st] : gs_units.all() ) {
    if( st.ownership.holds<UnitOwnership::old_world>() )
      res.push_back( id );
  }
  return res;
}

/****************************************************************
** Multi
*****************************************************************/
maybe<Coord> coord_for_unit_multi_ownership( UnitId id ) {
  if( auto maybe_map = coord_for_unit_indirect( id ); maybe_map )
    return maybe_map;
  if( auto maybe_colony = colony_for_unit_who_is_worker( id ) )
    return colony_from_id( *maybe_colony ).location();
  return nothing;
}

Coord coord_for_unit_multi_ownership_or_die( UnitId id ) {
  UNWRAP_CHECK( res, coord_for_unit_multi_ownership( id ) );
  return res;
}

/****************************************************************
** For Testing / Development Only
*****************************************************************/
UnitId create_unit_on_map( UnitsState& units_state,
                           e_nation nation, UnitComposition comp,
                           Coord coord ) {
  Unit& unit = units_state.unit_for(
      create_unit( units_state, nation, std::move( comp ) ) );
  units_state.change_to_map( unit.id(), coord );
  return unit.id();
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( create_unit_on_map, Unit&, e_nation nation,
        UnitComposition& comp, Coord const& coord ) {
  UnitsState& units_state = GameState::units();
  auto        id =
      create_unit_on_map( units_state, nation, comp, coord );
  lg.info( "created a {} on square {}.",
           unit_attr( comp.type() ).name, coord );
  auto& gs_units = GameState::units();
  return gs_units.unit_for( id );
}

LUA_FN( unit_from_id, Unit&, UnitId id ) {
  return unit_from_id( id );
}

LUA_FN( coord_for_unit, maybe<Coord>, UnitId id ) {
  return coord_for_unit( id );
}

LUA_FN( units_from_coord, lua::table, Coord c ) {
  lua::state& st  = lua_global_state();
  lua::table  res = st.table.create();
  int         i   = 1;
  for( UnitId id : units_from_coord( c ) ) res[i++] = id;
  return res;
}

// TODO: move this?
LUA_FN( last_unit_id, UnitId ) {
  auto& gs_units = GameState::units();
  return gs_units.last_unit_id();
}

} // namespace

} // namespace rn
