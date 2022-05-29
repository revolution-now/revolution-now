/****************************************************************
**on-map.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-18.
*
* Description: Handles actions that need to be take in response
*              to a unit appearing on a map square (after
*              creation or moving).
*
*****************************************************************/
#include "on-map.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "gs-terrain.hpp"
#include "gs-units.hpp"
#include "igui.hpp"
#include "lcr.hpp"
#include "logger.hpp"
#include "player.hpp"

// config
#include "config/nation.rds.hpp"

using namespace std;

namespace rn {

namespace {

string new_world_name_for( Player const& player ) {
  return config_nation.nations[player.nation()].new_world_name;
}

wait<> try_discover_new_world( TerrainState const& terrain_state,
                               Player& player, IGui& gui,
                               Coord world_square ) {
  // This field holds the name of the new world given by the
  // player if it has a value (meaning, if the new world has been
  // discovered).
  maybe<string>& new_world_name = player.discovered_new_world();
  if( new_world_name.has_value() ) co_return;
  for( e_direction d : refl::enum_values<e_direction> ) {
    maybe<MapSquare const&> square =
        terrain_state.maybe_square_at( world_square.moved( d ) );
    if( !square.has_value() ) continue;
    if( square->surface != e_surface::land ) continue;
    // We've discovered the new world!
    string name = co_await gui.string_input(
        { .msg = "You've discovered the new world!  What shall "
                 "we call this land, Your Excellency?",
          .initial_text = new_world_name_for( player ) } );
    new_world_name = name;
    lg.info( "the new world has been discovered: \"{}\".",
             name );
    CHECK( player.discovered_new_world().has_value() );
    co_return;
  }
}

wait<> try_lost_city_rumor( UnitsState&          units_state,
                            TerrainState const&  terrain_state,
                            Player&              player,
                            SettingsState const& settings,
                            IGui& gui, IMapUpdater& map_updater,
                            UnitId id, Coord world_square ) {
  // Check if the unit actually moved and it landed on a Lost
  // City Rumor.
  if( !has_lost_city_rumor( terrain_state, world_square ) )
    co_return;
  e_lcr_explorer_category const explorer =
      lcr_explorer_category( units_state, id );
  e_rumor_type rumor_type =
      pick_rumor_type_result( explorer, player );
  e_burial_mounds_type burial_type =
      pick_burial_mounds_result( explorer );
  bool has_burial_grounds = pick_burial_grounds_result(
      player, explorer, burial_type );
  LostCityRumorResult_t lcr_res =
      co_await run_lost_city_rumor_result(
          units_state, gui, player, settings, map_updater, id,
          world_square, rumor_type, burial_type,
          has_burial_grounds );

  // Not currently doing anything with the results.
  (void)lcr_res;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void unit_to_map_square_no_ui( UnitsState& units_state,
                               IMapUpdater&, UnitId id,
                               Coord world_square ) {
  // 1. Move the unit. This is the only place where this function
  // should be called by normal game code.
  units_state.change_to_map( id, world_square );

  // 2. Unsentry surrounding foreign units.
  // TODO

  // 3. Update terrain visibility.
  // TODO
}

wait<> unit_to_map_square( UnitsState&          units_state,
                           TerrainState const&  terrain_state,
                           Player&              player,
                           SettingsState const& settings,
                           IGui& gui, IMapUpdater& map_updater,
                           UnitId id, Coord world_square ) {
  unit_to_map_square_no_ui( units_state, map_updater, id,
                            world_square );

  if( !player.discovered_new_world().has_value() )
    co_await try_discover_new_world( terrain_state, player, gui,
                                     world_square );

  if( has_lost_city_rumor( terrain_state, world_square ) )
    co_await try_lost_city_rumor(
        units_state, terrain_state, player, settings, gui,
        map_updater, id, world_square );

  // !! Note that the LCR may have removed the unit!
}

} // namespace rn
