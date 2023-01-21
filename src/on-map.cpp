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
#include "igui.hpp"
#include "imap-updater.hpp"
#include "lcr.hpp"
#include "logger.hpp"
#include "meet-natives.hpp"
#include "treasure.hpp"
#include "ts.hpp"
#include "visibility.hpp"
#include "woodcut.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// config
#include "config/nation.rds.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

string new_world_name_for( Player const& player ) {
  return config_nation.nations[player.nation].new_world_name;
}

wait<> try_discover_new_world( SSConst const& ss, TS& ts,
                               Player& player,
                               Coord   world_square ) {
  // This field holds the name of the new world given by the
  // player if it has a value (meaning, if the new world has been
  // discovered).
  maybe<string> const& new_world_name = player.new_world_name;
  if( new_world_name.has_value() ) co_return;
  for( e_direction d : refl::enum_values<e_direction> ) {
    maybe<MapSquare const&> square =
        ss.terrain.maybe_square_at( world_square.moved( d ) );
    if( !square.has_value() ) continue;
    if( square->surface != e_surface::land ) continue;
    // We've discovered the new world!
    co_await display_woodcut_if_needed(
        ts, player, e_woodcut::discovered_new_world );
    string const name = co_await ts.gui.required_string_input(
        { .msg = "You've discovered the new world!  What shall "
                 "we call this land, Your Excellency?",
          .initial_text = new_world_name_for( player ) } );
    player.new_world_name = name;
    lg.info( "the new world has been discovered: \"{}\".",
             name );
    CHECK( player.new_world_name.has_value() );
    co_return;
  }
}

// Returns true if the unit was deleted.
wait<bool> try_lost_city_rumor( SS& ss, TS& ts, Player& player,
                                UnitId id, Coord world_square ) {
  // Check if the unit actually moved and it landed on a Lost
  // City Rumor.
  if( !has_lost_city_rumor( ss.terrain, world_square ) )
    co_return false;
  e_lcr_explorer_category const explorer =
      lcr_explorer_category( ss.units, id );
  e_rumor_type rumor_type =
      pick_rumor_type_result( ts.rand, explorer, player );
  e_burial_mounds_type burial_type =
      pick_burial_mounds_result( ts.rand, explorer );
  bool has_burial_grounds = pick_burial_grounds_result(
      ts.rand, player, explorer, burial_type );
  LostCityRumorResult_t lcr_res =
      co_await run_lost_city_rumor_result(
          ss, ts, player, id, world_square, rumor_type,
          burial_type, has_burial_grounds );

  co_return lcr_res.holds<LostCityRumorResult::unit_lost>();
}

// Returns true if the treasure was transported by the king and
// thus deleted.
wait<bool> try_king_transport_treasure( SS& ss, TS& ts,
                                        Player&     player,
                                        Unit const& unit,
                                        Coord world_square ) {
  if( unit.type() != e_unit_type::treasure ) co_return false;
  maybe<ColonyId> const colony_id =
      ss.colonies.maybe_from_coord( world_square );
  if( !colony_id.has_value() ) co_return false;
  Colony const& colony = ss.colonies.colony_for( *colony_id );
  CHECK_EQ( colony.nation, player.nation );
  maybe<TreasureReceipt> const receipt =
      co_await treasure_enter_colony( ss, ts, player, unit );
  if( !receipt.has_value() ) co_return false;
  apply_treasure_reimbursement( ss, player, *receipt );
  // !! Treasure unit has been deleted here.
  co_await show_treasure_receipt( ts, player, *receipt );
  co_return true; // treasure unit deleted.
}

wait<> try_meet_natives( SS& ss, TS& ts, Player& player,
                         Coord square ) {
  vector<MeetTribe> const meet_tribes =
      check_meet_tribes( as_const( ss ), player, square );
  for( MeetTribe const& meet_tribe : meet_tribes ) {
    e_declare_war_on_natives const declare_war =
        co_await perform_meet_tribe_ui_sequence( ts, player,
                                                 meet_tribe );
    perform_meet_tribe( ss, player, meet_tribe, declare_war );
  }
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void unit_to_map_square_non_interactive( SS& ss, TS& ts,
                                         UnitId id,
                                         Coord  world_square ) {
  Unit& unit = ss.units.unit_for( id );

  // 1. Move the unit. This is the only place where this function
  //    should be called by normal game code.
  ss.units.change_to_map( id, world_square );

  // 2. Unsentry surrounding foreign units.
  //    TODO

  // 3. Update terrain visibility.
  e_nation const      nation  = unit.nation();
  vector<Coord> const visible = unit_visible_squares(
      ss, nation, unit.type(), world_square );
  for( Coord coord : visible )
    ts.map_updater.make_square_visible( coord, nation );

  // 4. If the unit is at a colony site then append the unit ID
  //    to the colony's list of unit's at the gate (said list
  //    must be added to the data model) so that the ordering of
  //    units is predictable and stable when rendering them at
  //    the gate. NOTE: this method is called when a unit that is
  //    already at the gate changes type (e.g. by dragging horses
  //    onto it to make it a scout), and so in that case the unit
  //    will already be in that list and should not be moved or
  //    added again.
  //    TODO
}

void unit_to_map_square_non_interactive(
    SS& ss, NativeUnitId id, Coord world_square,
    DwellingId dwelling_id ) {
  // 1. Move the unit. This is the only place where this function
  //    should be called by normal game code.
  ss.units.change_to_map( id, world_square, dwelling_id );
}

wait<maybe<UnitDeleted>> unit_to_map_square(
    SS& ss, TS& ts, UnitId id, Coord world_square ) {
  // FIXME: need to be defensive here and check that the unit is
  // not being created on a square containing foreign entities,
  // otherwise other things will check-fail. Though there could
  // be exceptions to this, e.g. when an indian village is de-
  // stroyed and there emerges either a treasure train or a mis-
  // sionary unit before the village is deleted.

  unit_to_map_square_non_interactive( ss, ts, id, world_square );

  Unit& unit = ss.units.unit_for( id );
  UNWRAP_CHECK( player, ss.players.players[unit.nation()] );

  if( !player.new_world_name.has_value() )
    co_await try_discover_new_world( ss, ts, player,
                                     world_square );

  if( has_lost_city_rumor( ss.terrain, world_square ) )
    if( co_await try_lost_city_rumor( ss, ts, player, id,
                                      world_square ) )
      co_return UnitDeleted{};

  if( co_await try_king_transport_treasure( ss, ts, player, unit,
                                            world_square ) )
    // The unit was a treasure train, it entered a colony square,
    // the king asked to transport it, the player accepted, and
    // the treasure unit was deleted.
    co_return UnitDeleted{};

  co_await try_meet_natives( ss, ts, player, world_square );

  // Unit is still alive.
  co_return nothing;
}

} // namespace rn
