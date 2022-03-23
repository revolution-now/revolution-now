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
#include "cstate.hpp"
#include "enum.hpp"
#include "game-state.hpp"
#include "gs-units.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "rand.hpp"
#include "ustate.hpp"
#include "window.hpp"
#include "world-map.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

// base-util
#include "base-util/string.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
valid_or<e_new_colony_name_err> is_valid_new_colony_name(
    std::string_view name ) {
  if( colony_from_name( name ).has_value() )
    return invalid( e_new_colony_name_err::already_exists );
  return valid;
}

valid_or<e_found_colony_err> unit_can_found_colony(
    UnitId founder ) {
  using Res_t = e_found_colony_err;
  auto& unit  = unit_from_id( founder );

  if( unit.desc().ship )
    return invalid( Res_t::ship_cannot_found_colony );

  if( !unit.is_human() )
    return invalid( Res_t::non_human_cannot_found_colony );

  auto maybe_coord = coord_for_unit_indirect( founder );
  if( !maybe_coord.has_value() )
    return invalid( Res_t::colonist_not_on_map );

  if( colony_from_coord( *maybe_coord ) )
    return invalid( Res_t::colony_exists_here );

  if( !is_land( *maybe_coord ) )
    return invalid( Res_t::no_water_colony );

  return valid;
}

ColonyId found_colony_unsafe( UnitId           founder,
                              std::string_view name ) {
  if( auto res = is_valid_new_colony_name( name ); !res )
    // FIXME: improve error message generation.
    FATAL( "Cannot found colony, error code: {}.",
           refl::enum_value_name( res.error() ) );

  if( auto res = unit_can_found_colony( founder ); !res )
    // FIXME: improve error message generation.
    FATAL( "Cannot found colony, error code: {}.",
           refl::enum_value_name( res.error() ) );

  Unit& unit   = unit_from_id( founder );
  auto  nation = unit.nation();
  UNWRAP_CHECK( where, coord_for_unit_indirect( founder ) );

  // 1. Create colony object.
  ColonyId col_id = create_colony( nation, where, name );
  Colony&  col    = colony_from_id( col_id );

  // Strip unit of commodities and modifiers and put the commodi-
  // ties into the colony.
  col.strip_unit_commodities( founder );

  // 2. Find initial job for founder. (TODO)
  ColonyJob_t job =
      ColonyJob::mfg{ .mfg_job = e_mfg_job::bells };

  // 3. Move unit into it.
  GameState::units().change_to_colony( founder, col_id, job );

  // 5. Done.
  auto& desc = nation_obj( nation );
  lg.info( "created {} {} colony at {}.", desc.article,
           desc.adjective, where );

  // 6. Let Lua do anything that it needs to the colony.
  CHECK_HAS_VALUE(
      lua_global_state()["colony_mgr"]["on_founded_colony"]
          .pcall( col ) );

  return col_id;
}

wait<> evolve_colony_one_turn( ColonyId id ) {
  auto& colony = colony_from_id( id );
  lg.debug( "evolving colony: {}.", colony );
  auto& commodities = colony.commodities();
  commodities[e_commodity::food] +=
      rng::between( 3, 7, rng::e_interval::closed );
  if( commodities[e_commodity::food] >= 200 ) {
    commodities[e_commodity::food] -= 200;
    // FIXME: When creating a new unit on the map, decide whether
    // we need to unsentry surrounding units and, if so, find a
    // good place to put that such that it can be reused anytime
    // a unit is created on the map.
    UnitType colonist =
        UnitType::create( e_unit_type::free_colonist );
    auto unit_id = create_unit( colony.nation(), colonist );
    GameState::units().change_to_map( unit_id,
                                      colony.location() );
    co_await landview_ensure_visible( colony.location() );
    ui::e_ok_cancel answer = co_await ui::ok_cancel( fmt::format(
        "The @[H]{}@[] colony has produced a new colonist.  "
        "View colony?",
        colony.name() ) );
    if( answer == ui::e_ok_cancel::ok )
      co_await show_colony_view( id );
  }
}

void change_colony_nation( ColonyId id, e_nation new_nation ) {
  unordered_set<UnitId> units = units_at_or_in_colony( id );
  for( UnitId unit_id : units )
    unit_from_id( unit_id ).change_nation( new_nation );
  auto& colony = colony_from_id( id );
  CHECK( colony.nation() != new_nation );
  colony.set_nation( new_nation );
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
LUA_FN( found_colony, ColonyId, UnitId founder,
        std::string const& name ) {
  if( auto res = is_valid_new_colony_name( name ); !res )
    // FIXME: improve error message generation.
    st.error( "cannot found colony here: {}.",
              enum_to_display_name( res.error() ) );
  if( auto res = unit_can_found_colony( founder ); !res )
    st.error( "cannot found colony here." );
  return found_colony_unsafe( founder, name );
}

} // namespace

} // namespace rn
