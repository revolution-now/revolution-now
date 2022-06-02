/****************************************************************
**colony-mgr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-01.
*
* Description: Main interface for controlling Colonies.
*
*****************************************************************/
#include "colony-mgr.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-view.hpp"
#include "colony.hpp"
#include "enum.hpp"
#include "game-state.hpp"
#include "gs-colonies.hpp"
#include "gs-players.hpp"
#include "gs-terrain.hpp"
#include "gs-units.hpp"
#include "igui.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "on-map.hpp"
#include "player.hpp"
#include "production.hpp"
#include "rand.hpp"
#include "road.hpp"
#include "ustate.hpp"
#include "window.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/to-str-ext-std.hpp"

// base-util
#include "base-util/string.hpp"

using namespace std;

namespace rn {

namespace {

// This returns all units that are either working in the colony
// or who are on the map on the colony square.
unordered_set<UnitId> units_at_or_in_colony(
    Colony const& colony, UnitsState const& units_state ) {
  unordered_set<UnitId> all =
      units_state.from_colony( colony.id() );
  Coord colony_loc = colony.location();
  for( UnitId map_id : units_from_coord( colony_loc ) )
    all.insert( map_id );
  return all;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
ColonyId create_empty_colony( ColoniesState& colonies_state,
                              e_nation nation, Coord where,
                              string_view name ) {
  return colonies_state.add_colony( Colony( wrapped::Colony{
      .nation   = nation,
      .name     = string( name ),
      .location = where,
  } ) );
}

valid_or<e_new_colony_name_err> is_valid_new_colony_name(
    ColoniesState const& colonies_state, string_view name ) {
  if( colonies_state.maybe_from_name( name ).has_value() )
    return invalid( e_new_colony_name_err::already_exists );
  return valid;
}

valid_or<e_found_colony_err> unit_can_found_colony(
    ColoniesState const& colonies_state,
    UnitsState const&    units_state,
    TerrainState const& terrain_state, UnitId founder ) {
  using Res_t      = e_found_colony_err;
  Unit const& unit = units_state.unit_for( founder );

  if( unit.desc().ship )
    return invalid( Res_t::ship_cannot_found_colony );

  if( !unit.is_human() )
    return invalid( Res_t::non_human_cannot_found_colony );

  auto maybe_coord =
      coord_for_unit_indirect( units_state, founder );
  if( !maybe_coord.has_value() )
    return invalid( Res_t::colonist_not_on_map );

  if( colonies_state.maybe_from_coord( *maybe_coord ) )
    return invalid( Res_t::colony_exists_here );

  // Check if we are too close to another colony.
  for( e_direction d : refl::enum_values<e_direction> ) {
    // Note that at this point we already know that there is no
    // colony on the center square.
    Coord new_coord = maybe_coord->moved( d );
    if( !terrain_state.square_exists( new_coord ) ) continue;
    if( colonies_state.maybe_from_coord(
            maybe_coord->moved( d ) ) )
      return invalid( Res_t::too_close_to_colony );
  }

  if( !terrain_state.is_land( *maybe_coord ) )
    return invalid( Res_t::no_water_colony );

  return valid;
}

ColonyId found_colony_unsafe( ColoniesState&      colonies_state,
                              TerrainState const& terrain_state,
                              UnitsState&         units_state,
                              UnitId              founder,
                              IMapUpdater&        map_updater,
                              string_view         name ) {
  if( auto res =
          is_valid_new_colony_name( colonies_state, name );
      !res )
    // FIXME: improve error message generation.
    FATAL( "Cannot found colony, error code: {}.",
           refl::enum_value_name( res.error() ) );

  if( auto res = unit_can_found_colony(
          colonies_state, units_state, terrain_state, founder );
      !res )
    // FIXME: improve error message generation.
    FATAL( "Cannot found colony, error code: {}.",
           refl::enum_value_name( res.error() ) );

  Unit& unit   = units_state.unit_for( founder );
  auto  nation = unit.nation();
  UNWRAP_CHECK(
      where, coord_for_unit_indirect( units_state, founder ) );

  // Create colony object.
  ColonyId col_id =
      create_empty_colony( colonies_state, nation, where, name );
  Colony& col = colonies_state.colony_for( col_id );

  // Strip unit of commodities and modifiers and put the commodi-
  // ties into the colony.
  strip_unit_commodities( units_state, unit, col );

  // Find initial job for founder. (TODO)
  ColonyJob_t job =
      ColonyJob::indoor{ .job = e_indoor_job::bells };

  // Move unit into it.
  move_unit_to_colony( units_state, col, founder, job );

  // Add road onto colony square.
  set_road( map_updater, where );

  // Done.
  auto& desc = nation_obj( nation );
  lg.info( "created {} {} colony at {}.", desc.article,
           desc.adjective, where );

  // Let Lua do anything that it needs to the colony.
  CHECK_HAS_VALUE(
      lua_global_state()["colony_mgr"]["on_founded_colony"]
          .pcall( col ) );

  return col_id;
}

wait<> evolve_colony_one_turn( Colony&              colony,
                               SettingsState const& settings,
                               UnitsState&          units_state,
                               TerrainState const& terrain_state,
                               PlayersState&       players_state,
                               IMapUpdater&        map_updater,
                               IGui&               gui ) {
  UNWRAP_CHECK( player, base::lookup( players_state.players,
                                      colony.nation() ) );
  ColonyId id = colony.id();
  lg.debug( "evolving colony: {}.", colony );
  auto& commodities = colony.commodities();

  // Production.
  // struct ColonyProduction {
  //   refl::enum_map<e_colony_product, int> produced;
  //   refl::enum_map<e_commodity, int>      consumed;
  // };
  ColonyProduction production = production_for_colony(
      units_state, players_state, colony );
  // FIXME: temporary
  player.crosses +=
      production.produced[e_colony_product::crosses];

#if 0
  commodities[e_commodity::food] +=
      rng::between( 3, 7, rng::e_interval::closed );
#endif

  if( commodities[e_commodity::food] >= 200 ) {
    commodities[e_commodity::food] -= 200;
    UnitType colonist =
        UnitType::create( e_unit_type::free_colonist );
    auto unit_id =
        create_unit( units_state, colony.nation(), colonist );
    Player& player = player_for_nation( GameState::players(),
                                        colony.nation() );
    co_await unit_to_map_square(
        units_state, terrain_state, player, settings, gui,
        map_updater, unit_id, colony.location() );
    co_await landview_ensure_visible( colony.location() );
    ui::e_ok_cancel answer = co_await ui::ok_cancel( fmt::format(
        "The @[H]{}@[] colony has produced a new colonist.  "
        "View colony?",
        colony.name() ) );
    if( answer == ui::e_ok_cancel::ok )
      co_await show_colony_view( id, map_updater );
  }
}

void change_colony_nation( Colony&     colony,
                           UnitsState& units_state,
                           e_nation    new_nation ) {
  unordered_set<UnitId> units =
      units_at_or_in_colony( colony, units_state );
  for( UnitId unit_id : units )
    units_state.unit_for( unit_id ).change_nation( new_nation );
  CHECK( colony.nation() != new_nation );
  colony.set_nation( new_nation );
}

void strip_unit_commodities( UnitsState const& units_state,
                             Unit& unit, Colony& colony ) {
  UNWRAP_CHECK_MSG(
      coord, units_state.maybe_coord_for( unit.id() ),
      "unit must be on map to shed its commodities." );
  CHECK( coord == colony.location(),
         "unit must be in colony to shed its commodities." );
  UnitTransformationResult tranform_res =
      unit.strip_to_base_type();
  for( auto [type, q] : tranform_res.commodity_deltas ) {
    CHECK_GT( q, 0 );
    lg.debug( "adding {} {} to colony {}.", q, type,
              colony.name() );
    colony.commodities()[type] += q;
  }
}

void move_unit_to_colony( UnitsState& units_state,
                          Colony& colony, UnitId unit_id,
                          ColonyJob_t const& job ) {
  CHECK( units_state.unit_for( unit_id ).nation() ==
         colony.nation() );
  units_state.change_to_colony( unit_id, colony.id() );
  colony.add_unit( unit_id, job );
}

void remove_unit_from_colony( UnitsState& units_state,
                              Colony& colony, UnitId unit_id ) {
  CHECK( units_state.unit_for( unit_id ).nation() ==
         colony.nation() );
  units_state.disown_unit( unit_id );
  colony.remove_unit( unit_id );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// FIXME: calling this function on the blinking unit will cause
// errors or check-fails in the game; the proper way to do this
// is to have a mechanism by which we can inject player commands
// as if the player had pressed 'b' so that the game can process
// the fact that the unit in question is now in a colony.
//
// This function is also currently used to setup some colonies
// from Lua at startup, where it works fine. The safer way to do
// that would be to have a single function that both creates a
// unit and the colony together.
//
// FIXME: this currently does not update the rendered map because
// it breaks unit tests where there is no global renderer -- that
// needs to be fixed.
LUA_FN( found_colony, ColonyId, UnitId founder,
        string const& name ) {
  ColoniesState& colonies_state = GameState::colonies();
  TerrainState&  terrain_state  = GameState::terrain();
  UnitsState&    units_state    = GameState::units();
  if( auto res =
          is_valid_new_colony_name( colonies_state, name );
      !res )
    // FIXME: improve error message generation.
    st.error( "cannot found colony here: {}.",
              enum_to_display_name( res.error() ) );
  if( auto res = unit_can_found_colony(
          colonies_state, units_state, terrain_state, founder );
      !res )
    st.error( "cannot found colony here." );
  // FIXME: needs to render.
  NonRenderingMapUpdater map_updater( terrain_state );
  return found_colony_unsafe( colonies_state, terrain_state,
                              units_state, founder, map_updater,
                              name );
}

} // namespace

} // namespace rn
