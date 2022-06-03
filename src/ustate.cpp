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
#include "co-wait.hpp"
#include "colony.hpp"
#include "cstate.hpp"
#include "error.hpp"
#include "game-state.hpp"
#include "gs-units.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "macros.hpp"
#include "on-map.hpp"
#include "player.hpp"
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
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::function_ref;

// If the unit is working in the colony then this will return it;
// however it will not return a ColonyId if the unit simply occu-
// pies the same square as the colony.
maybe<ColonyId> colony_for_unit_who_is_worker( UnitId id ) {
  auto const&     gs_units = GameState::units();
  maybe<ColonyId> res;
  if_get( gs_units.ownership_of( id ), UnitOwnership::colony,
          colony_state ) {
    return colony_state.id;
  }
  return res;
}

} // namespace

/****************************************************************
** Units
*****************************************************************/
string debug_string( UnitsState const& units_state, UnitId id ) {
  return debug_string( units_state.unit_for( id ) );
}

Unit& unit_from_id( UnitId id ) {
  return GameState::units().unit_for( id );
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

Unit create_free_unit( e_nation nation, UnitComposition comp ) {
  wrapped::Unit refl_unit{
      .id          = UnitId{ 0 }, // will be set later.
      .composition = std::move( comp ),
      .orders      = e_unit_orders::none,
      .cargo = CargoHold( unit_attr( comp.type() ).cargo_slots ),
      .nation = nation,
      .mv_pts = unit_attr( comp.type() ).movement_points,
  };
  return Unit( std::move( refl_unit ) );
}

UnitId create_unit( UnitsState& units_state, e_nation nation,
                    UnitType type ) {
  return create_unit( units_state, nation,
                      UnitComposition::create( type ) );
}

UnitId create_unit( UnitsState& units_state, e_nation nation,
                    e_unit_type type ) {
  return create_unit( units_state, nation,
                      UnitType::create( type ) );
}

UnitId create_unit_on_map_no_ui( UnitsState&     units_state,
                                 IMapUpdater&    map_updater,
                                 e_nation        nation,
                                 UnitComposition comp,
                                 Coord           coord ) {
  UnitId id =
      create_unit( units_state, nation, std::move( comp ) );
  unit_to_map_square_no_ui( units_state, map_updater, id,
                            coord );
  return id;
}

wait<UnitId> create_unit_on_map(
    UnitsState& units_state, TerrainState const& terrain_state,
    Player& player, SettingsState const& settings, IGui& gui,
    IMapUpdater& map_updater, UnitComposition comp,
    Coord coord ) {
  UnitId id = create_unit( units_state, player.nation,
                           std::move( comp ) );
  co_await unit_to_map_square( units_state, terrain_state,
                               player, settings, gui,
                               map_updater, id, coord );
  co_return id;
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

maybe<Coord> coord_for_unit( UnitId id ) {
  return GameState::units().maybe_coord_for( id );
}

Coord coord_for_unit_indirect_or_die( UnitId id ) {
  UNWRAP_CHECK( res, coord_for_unit_indirect( id ) );
  return res;
}

// If this function makes recursive calls it should always call
// the _safe variant since this function should not throw.
maybe<Coord> coord_for_unit_indirect(
    UnitsState const& units_state, UnitId id ) {
  CHECK( units_state.exists( id ) );
  UnitOwnership_t const& ownership =
      units_state.ownership_of( id );
  switch( ownership.to_enum() ) {
    case UnitOwnership::e::world: {
      auto& [coord] = ownership.get<UnitOwnership::world>();
      return coord;
    }
    case UnitOwnership::e::cargo: {
      auto& [holder] = ownership.get<UnitOwnership::cargo>();
      return coord_for_unit_indirect( units_state, holder );
    }
    case UnitOwnership::e::free:
    case UnitOwnership::e::harbor:
    case UnitOwnership::e::colony: //
      return nothing;
  };
}

// If this function makes recursive calls it should always call
// the _safe variant since this function should not throw.
maybe<Coord> coord_for_unit_indirect( UnitId id ) {
  auto const& units_state = GameState::units();
  return coord_for_unit_indirect( units_state, id );
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
vector<UnitId> units_in_harbor_view() {
  vector<UnitId> res;
  auto&          gs_units = GameState::units();
  for( auto const& [id, st] : gs_units.all() ) {
    if( st.ownership.holds<UnitOwnership::harbor>() )
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
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( create_unit_on_map, Unit&, e_nation nation,
        UnitComposition& comp, Coord const& coord ) {
  UnitsState& units_state = GameState::units();
  // FIXME: this needs to render but can't cause it causes
  // trouble for unit tests.
  NonRenderingMapUpdater map_updater( GameState::terrain() );
  auto id = create_unit_on_map_no_ui( units_state, map_updater,
                                      nation, comp, coord );
  lg.info( "created a {} on square {}.",
           unit_attr( comp.type() ).name, coord );
  auto& gs_units = GameState::units();
  return gs_units.unit_for( id );
}

LUA_FN( add_unit_to_cargo, void, UnitId held, UnitId holder ) {
  UnitsState& units_state = GameState::units();
  lg.info( "adding unit {} to cargo of unit {}.",
           debug_string( units_state, held ),
           debug_string( units_state, holder ) );
  units_state.change_to_cargo_somewhere( holder, held );
}

LUA_FN( create_unit_in_cargo, Unit&, e_nation nation,
        UnitComposition& comp, UnitId holder ) {
  UnitsState& units_state = GameState::units();
  UnitId      unit_id = create_unit( units_state, nation, comp );
  lg.info( "created unit {}.",
           debug_string( units_state, unit_id ),
           debug_string( units_state, holder ) );
  units_state.change_to_cargo_somewhere( holder, unit_id );
  return units_state.unit_for( unit_id );
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
