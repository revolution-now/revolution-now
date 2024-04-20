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
#include "ieuro-mind.hpp"
#include "igui.hpp"
#include "imap-search.rds.hpp"
#include "imap-updater.hpp"
#include "lcr.hpp"
#include "logger.hpp"
#include "meet-natives.hpp"
#include "minds.hpp"
#include "society.hpp"
#include "treasure.hpp"
#include "tribe-arms.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unsentry.hpp"
#include "visibility.hpp"
#include "woodcut.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/tribe.rds.hpp"
#include "ss/units.hpp"

// config
#include "config/nation.rds.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/vocab.hpp"

using namespace std;

namespace rn {

namespace {

string new_world_name_for( Player const& player ) {
  return config_nation.nations[player.nation].new_world_name;
}

wait<> try_discover_new_world( SSConst const& ss, TS& ts,
                               Player&    player,
                               IEuroMind& euro_mind,
                               Coord      world_square ) {
  // This field holds the name of the new world given by the
  // player if it has a value (meaning, if the new world has been
  // discovered).
  maybe<string> const& new_world_name = player.new_world_name;
  if( new_world_name.has_value() ) co_return;
  // Need to use cdirection since if you put a colonist on an is-
  // land they otherwise wouldn't discover the new world. Actu-
  // ally islands are removed from the map since they allow a
  // form of cheating, but there is one on the America map and
  // they can be created in the map editor, so we need to do the
  // right thing.
  for( e_cdirection d : refl::enum_values<e_cdirection> ) {
    maybe<MapSquare const&> square =
        ss.terrain.maybe_square_at( world_square.moved( d ) );
    if( !square.has_value() ) continue;
    if( square->surface != e_surface::land ) continue;
    // We've discovered the new world!
    co_await show_woodcut_if_needed(
        player, euro_mind, e_woodcut::discovered_new_world );
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

wait<> try_discover_pacific_ocean( SSConst const& ss,
                                   Player&        player,
                                   IEuroMind&     euro_mind,
                                   Coord world_square ) {
  for( e_direction d : refl::enum_values<e_direction> ) {
    Coord const coord = world_square.moved( d );
    if( !ss.terrain.square_exists( coord ) ) continue;
    if( !ss.terrain.is_pacific_ocean( coord ) ) continue;
    // We've discovered the Pacific Ocean!
    co_await show_woodcut_if_needed(
        player, euro_mind, e_woodcut::discovered_pacific_ocean );
    lg.info( "the pacific ocean been discovered." );
    break;
  }
}

// Returns true if the unit was deleted.
wait<base::NoDiscard<bool>> try_lost_city_rumor(
    SS& ss, TS& ts, Player& player, IEuroMind& euro_mind,
    UnitId id, Coord tile ) {
  // Check if the unit actually moved and it landed on a Lost
  // City Rumor.
  bool const has_lost_city_rumor =
      ss.terrain.square_at( tile ).lost_city_rumor;
  if( !has_lost_city_rumor ) co_return false;
  Unit const&         unit  = ss.units.unit_for( id );
  LostCityRumor const rumor = compute_lcr(
      ss.as_const, player, ts.rand, RealMapSearch( ss.as_const ),
      unit.type(), tile );
  if( rumor.holds<LostCityRumor::fountain_of_youth>() )
    co_await show_woodcut_if_needed(
        player, euro_mind,
        e_woodcut::discovered_fountain_of_youth );
  LostCityRumorUnitChange const lcr_res =
      co_await run_lcr( ss, ts, player, unit, tile, rumor );
  co_return lcr_res.holds<LostCityRumorUnitChange::unit_lost>();
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

wait<> try_meet_natives( SS& ss, Player& player,
                         IEuroMind& euro_mind, Coord square ) {
  vector<MeetTribe> const meet_tribes =
      check_meet_tribes( as_const( ss ), player, square );
  for( MeetTribe const& meet_tribe : meet_tribes ) {
    e_declare_war_on_natives const declare_war =
        co_await euro_mind.meet_tribe_ui_sequence( meet_tribe );
    perform_meet_tribe( ss, player, meet_tribe, declare_war );
  }
}

wait<> try_meet_europeans( SS& ss, TS& ts, e_tribe tribe_type,
                           Coord native_tile ) {
  vector<MeetTribe> const meet_tribes = check_meet_europeans(
      as_const( ss ), tribe_type, native_tile );
  for( MeetTribe const& meet_tribe : meet_tribes ) {
    Player& player = player_for_nation_or_die(
        ss.players, meet_tribe.nation );
    e_declare_war_on_natives const declare_war =
        co_await ts.euro_minds()[meet_tribe.nation]
            .meet_tribe_ui_sequence( meet_tribe );
    perform_meet_tribe( ss, player, meet_tribe, declare_war );
  }
}

[[nodiscard]] bool try_perform_inter_tribe_trade_impl(
    SS& ss, Tribe& unit_tribe, Coord center_tile ) {
  Tribe& tribe1          = unit_tribe;
  bool   trade_performed = false;
  for( e_direction d : refl::enum_values<e_direction> ) {
    Coord const surrounding_tile = center_tile.moved( d );
    maybe<MapSquare const&> square =
        ss.terrain.maybe_square_at( surrounding_tile );
    if( !square.has_value() ) continue;
    maybe<Society> const society =
        society_on_square( ss, surrounding_tile );
    if( !society.has_value() ) continue;
    SWITCH( *society ) {
      CASE( european ) { break; }
      CASE( native ) {
        if( native.tribe == unit_tribe.type ) continue;
        Tribe& tribe2 = ss.natives.tribe_for( native.tribe );
        // This method should be idempotent.
        if( perform_inter_tribe_trade( tribe1, tribe2 ) )
          trade_performed = true;
        break;
      }
    }
  }
  return trade_performed;
}

void try_perform_inter_tribe_trade(
    SS& ss, NativeUnitId native_unit_id ) {
  Coord const unit_loc = ss.units.coord_for( native_unit_id );
  NativeUnit const& native_unit =
      ss.units.unit_for( native_unit_id );
  Tribe& unit_tribe = tribe_for_unit( ss, native_unit );

  // We need to do this twice due to the way that the inter-tribe
  // trading mechanism works. When two tribes trade, their
  // horse_herds (for both tribes) will become the max of the
  // two. This means that if it happens (rare) that there are
  // three tribes that are now adjacent, we need to do a second
  // pass just in case the second trade involves a higher quan-
  // tity than the first trade. In other words, if there tribes
  // are in contact, we want all three tribes to end up with the
  // same number of horse_herds, and that value should be the max
  // value of the three before the trades. And we need two passes
  // to ensure that when there are more than two total tribes in-
  // volved in the trade. That said, if the first pass doesn't do
  // any trading then there is no need to call the second.
  //
  // In the "normal" case where there are only two tribes in-
  // volved, the second one should be a no-op since overall
  // inter-tribe trading should be idempotent.
  if( try_perform_inter_tribe_trade_impl( ss, unit_tribe,
                                          unit_loc ) )
    // Second pass; guaranteed to be sufficient.
    (void)try_perform_inter_tribe_trade_impl( ss, unit_tribe,
                                              unit_loc );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void UnitOnMapMover::to_map_non_interactive(
    SS& ss, TS& ts, UnitId id, Coord world_square ) {
  Unit& unit = ss.units.unit_for( id );

  // 1. Adjust the visibility/fog in response to the unit moving
  // to the destination square and (potentially) leaving a source
  // square. Should be done before unit is moved.
  vector<Coord> const visible = unit_visible_squares(
      ss, unit.nation(), unit.type(), world_square );
  ts.map_updater().make_squares_visible( unit.nation(),
                                         visible );

  // 2. Move the unit. This is the only place where this function
  //    should be called by normal game code.
  ss.units.change_to_map( id, world_square );

  // 3. Unsentry surrounding foreign units.
  unsentry_foreign_units_next_to_euro_unit( ss, unit );

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

void UnitOnMapMover::native_unit_to_map_non_interactive(
    SS& ss, NativeUnitId id, Coord dst_tile ) {
  // 1. Move the unit. This is the only place where this function
  //    should be called by normal game code.
  ss.units.move_unit_on_map( id, dst_tile );

  // 2. Unsentry surrounding european units.
  unsentry_units_next_to_tile( ss, dst_tile );

  // 3. Perform inter-tribe trade if adjacent to another tribe.
  try_perform_inter_tribe_trade( ss, id );
}

wait<maybe<UnitDeleted>> UnitOnMapMover::to_map_interactive(
    SS& ss, TS& ts, UnitId id, Coord dst ) {
  to_map_non_interactive( ss, ts, id, dst );

  Unit& unit = ss.units.unit_for( id );
  UNWRAP_CHECK( player, ss.players.players[unit.nation()] );
  IEuroMind& euro_mind = ts.euro_minds()[player.nation];

  if( !player.new_world_name.has_value() )
    co_await try_discover_new_world( ss, ts, player, euro_mind,
                                     dst );

  if( !player.woodcuts[e_woodcut::discovered_pacific_ocean] )
    co_await try_discover_pacific_ocean( ss, player, euro_mind,
                                         dst );

  if( co_await try_lost_city_rumor( ss, ts, player, euro_mind,
                                    id, dst ) )
    co_return UnitDeleted{};

  if( co_await try_king_transport_treasure( ss, ts, player, unit,
                                            dst ) )
    // The unit was a treasure train, it entered a colony square,
    // the king asked to transport it, the player accepted, and
    // the treasure unit was deleted.
    co_return UnitDeleted{};

  co_await try_meet_natives( ss, player, euro_mind, dst );

  // Unit is still alive.
  co_return nothing;
}

wait<> UnitOnMapMover::native_unit_to_map_interactive(
    SS& ss, TS& ts, NativeUnitId id, Coord dst_tile ) {
  NativeUnit const& native_unit = ss.units.unit_for( id );
  e_tribe const     tribe_type =
      tribe_type_for_unit( ss, native_unit );
  native_unit_to_map_non_interactive( ss, id, dst_tile );

  co_await try_meet_europeans( ss, ts, tribe_type, dst_tile );
}

} // namespace rn
