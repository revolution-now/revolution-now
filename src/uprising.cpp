/****************************************************************
**uprising.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-08-15.
*
* Description: Implements the "Tory Uprising" mechanic.
*
*****************************************************************/
#include "uprising.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "connectivity.hpp"
#include "igui.hpp"
#include "irand.hpp"
#include "map-square.hpp"
#include "ref.hpp"
#include "society.hpp"
#include "sons-of-liberty.hpp"
#include "unit-mgr.hpp"

// config
#include "config/revolution.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/nation.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

// base
#include "base/range-lite.hpp"

using namespace std;

namespace rl = ::base::rl;

namespace rn {

namespace {

using ::gfx::point;
using ::refl::enum_values;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
bool should_attempt_uprising(
    SSConst const& ss, Player const& colonial_player,
    Player const& ref_player,
    bool const did_deploy_ref_this_turn ) {
  // Try for a "Tory Uprising". This only happens once no further
  // REF units can be sent, and assuming that none were sent just
  // now on this turn.
  return
      // In the OG an uprising can happen after the war ends and
      // the player opts to keep playing, but we are not doing
      // that here as it doesn't really make sense.
      colonial_player.revolution.status ==
          e_revolution_status::declared &&
      // We can't replace this condition with a test of whether
      // there are REF units on the map, because we really need
      // to test whether units were deployed this turn.
      !did_deploy_ref_this_turn &&
      // It may be that we didn't deploy any units this turn, but
      // e.g. that may be because some Man-o-Wars are returning
      // to europe to pick them up. We don't want to do uprisings
      // in that case.
      !can_send_more_ref_units( ss, colonial_player,
                                ref_player );
}

UprisingColonies find_uprising_colonies(
    SSConst const& ss, TerrainConnectivity const& connectivity,
    e_player const colonial_player_type ) {
  UprisingColonies res;
  UNWRAP_CHECK_T( Player const& colonial_player,
                  ss.players.players[colonial_player_type] );
  // Conditions for happening once attempted:
  //
  //   - Colony does /not/ need to be a port colony.
  //   - Colony has `had_tory_uprising=false`.
  //   - Colony is in the hands of the rebels.
  //   - Colony should not still be being attacked by normal REF
  //     troops.
  //   - There needs to be an unoccupied land square adjacent to
  //     the colony. The uprise will not displace units unlike
  //     with REF landings.
  //   - Colony has sufficiently low defenses. This is effec-
  //     tively enforced by computing the number of tories that
  //     will be spawned according to the formula (which factors
  //     in defenses) and ensuring that it is larger than zero.
  //
  // It appears to iterate through the colonies in order, rolling
  // dice for each based on SoL and/or defense strength. As soon
  // as there is one uprising it stops and doesn't do any further
  // colonies. If it gets through the colonies with no uprising
  // then none will happen that turn.
  vector<ColonyId> const colonies =
      ss.colonies.for_player_sorted( colonial_player_type );
  for( ColonyId const colony_id : colonies ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    if( colony.had_tory_uprising ) continue;
    int const population = colony_population( colony );
    UprisingColony up_colony{ .colony_id = colony_id };
    // Do the unit count first because it is likely to yield zero
    // units in most cases, in which we just skip this colony.
    // This is because it doesn't take many defensive units to
    // suppress the tories.
    int const num_tories = [&] {
      // TODO: check if this works for colony population = 0.
      double const sons_of_liberty_percent =
          compute_sons_of_liberty_percent(
              colony.sons_of_liberty.num_rebels_from_bells_only,
              population,
              colonial_player.fathers
                  .has[e_founding_father::simon_bolivar] );
      int const sons_of_liberty_integral_percent =
          compute_sons_of_liberty_integral_percent(
              sons_of_liberty_percent );
      int const sons_of_liberty_number =
          compute_sons_of_liberty_number(
              sons_of_liberty_integral_percent, population );
      return compute_tory_number( sons_of_liberty_number,
                                  population );
    }();
    int const defense_strength = [&] {
      int res = 0;
      vector<GenericUnitId> const units =
          units_from_coord_recursive( ss.units,
                                      colony.location );
      for( GenericUnitId const generic_id : units ) {
        switch( ss.units.unit_kind( generic_id ) ) {
          case e_unit_kind::euro:
            break;
          case e_unit_kind::native:
            // This should not happen in a well-formed game, but
            // let's be defensive here in case the situation ac-
            // cidentally comes up via cheat mode.
            continue;
        }
        Unit const& unit = ss.units.euro_unit_for( generic_id );
        if( !unit.desc().can_attack ) continue;
        res += unit.desc().combat;
      }
      return res;
    }();
    int const difficulty_term = [&] {
      switch( ss.settings.game_setup_options.difficulty ) {
        case e_difficulty::conquistador:
          return -2;
        case e_difficulty::discoverer:
          return -1;
        case e_difficulty::explorer:
          return 0;
        case e_difficulty::governor:
          return 1;
        case e_difficulty::viceroy:
          return 2;
      }
    }();

    // * Units chosen:
    //     - The unit count is given by:
    //         T*2 + 3 - S + D
    //       where T is the number of tories in the colony, S is
    //       the total strength of the units in the colony (ar-
    //       tillery=7, soldier=2, etc), and D is the difficulty
    //       term:
    //         discoverer:   -2
    //         explorer:     -1
    //         conquistador:  0
    //         governor:     +1
    //         viceroy:      +2
    //       There is no floor on it; if the above formula yields
    //       1, then only one unit will be deployed.
    up_colony.unit_count = std::max(
        num_tories * 2 + 3 - defense_strength + difficulty_term,
        0 );
    // This is used as a way to filter out colonies that are too
    // strong, since they will tend to have a more negative unit
    // count.
    if( up_colony.unit_count <= 0 ) continue;
    vector<point> tiles;
    tiles.reserve( 20 );
    point const center = colony.location.to_gfx();
    for( e_direction const d : enum_values<e_direction> )
      tiles.push_back( center.moved( d ) );
    // TODO: move this out.
    {
      using enum e_direction;
      // Top row.
      tiles.push_back( center.moved( n ).moved( n ) );
      tiles.push_back( center.moved( n ).moved( n ).moved( e ) );
      tiles.push_back( center.moved( n ).moved( n ).moved( w ) );
      // West row.
      tiles.push_back( center.moved( w ).moved( w ) );
      tiles.push_back( center.moved( w ).moved( w ).moved( n ) );
      tiles.push_back( center.moved( w ).moved( w ).moved( s ) );
      // East row.
      tiles.push_back( center.moved( e ).moved( e ) );
      tiles.push_back( center.moved( e ).moved( e ).moved( n ) );
      tiles.push_back( center.moved( e ).moved( e ).moved( s ) );
      // Bottom row.
      tiles.push_back( center.moved( s ).moved( s ) );
      tiles.push_back( center.moved( s ).moved( s ).moved( e ) );
      tiles.push_back( center.moved( s ).moved( s ).moved( w ) );
    }
    auto const not_connected = [&]( point const p ) {
      return !tiles_are_connected( connectivity, p,
                                   colony.location );
    };
    erase_if( tiles, not_connected );
    for( point const tile : tiles ) {
      if( !ss.terrain.square_exists( tile ) ) continue;
      MapSquare const& square = ss.terrain.square_at( tile );
      if( is_water( square ) ) continue;
      if( auto const society = society_on_square( ss, tile );
          society.has_value() )
        // If this is an REF unit then the colony is being at-
        // tacked by the REF already, so skip. Any other type of
        // unit (native, colonial player) cannot be displaced, so
        // skip there as well. Basically the tile has to be
        // empty.
        continue;
      if( tile.direction_to( colony.location ).has_value() )
        up_colony.available_tiles_adjacent.push_back( tile );
      else
        up_colony.available_tiles_beyond.push_back( tile );
    }
    if( up_colony.available_tiles_adjacent.empty() ) continue;
    res.colonies.push_back( std::move( up_colony ) );
  }
  return res;
}

UprisingColony const* select_uprising_colony(
    SSConst const& ss, IRand& rand,
    UprisingColonies const& uprising_colonies ) {
  // TODO
  // - Dice roll to determine whether the uprising happens, prob-
  //   ability seems related to SoL, but not sure.
  //
  // - Probability that it happens to a colony:
  //     * Zero if defenses lead to too few units.
  //     * Depends on difficulty level.
  //     * Not clear if it depends on SoL.
  //     * Since the exact formula is not known, we will use
  //       this:
  //
  //         P_colony = D + T/4
  //
  //       where D is:
  //
  //         discoverer:   25%
  //         explorer:     38%
  //         conquistador: 50%
  //         governor:     63%
  //         viceroy:      75%
  //
  //       and T is the tory percent in the colony / 4. So on
  //       Viceroy, if the tory percent is 100% then we have
  //       75%+100%/4 = 100%. On discoverer, if the Tory percent
  //       is 0% then we have 25%+0% = 25%. This is likely not
  //       the same formula that the OG uses, but it seems to
  //       roughly fit what was observed and should be good
  //       enough.
  //
  int const difficulty_term = [&] {
    switch( ss.settings.game_setup_options.difficulty ) {
      case e_difficulty::conquistador:
        return 25;
      case e_difficulty::discoverer:
        return 38;
      case e_difficulty::explorer:
        return 50;
      case e_difficulty::governor:
        return 63;
      case e_difficulty::viceroy:
        return 75;
    }
  }();
  for( UprisingColony const& up_colony :
       uprising_colonies.colonies ) {
    ColonyId const colony_id = up_colony.colony_id;
    Colony const& colony = ss.colonies.colony_for( colony_id );
    int const population = colony_population( colony );
    UNWRAP_CHECK_T( Player const& player,
                    ss.players.players[colony.player] );
    double const sons_of_liberty_percent =
        compute_sons_of_liberty_percent(
            colony.sons_of_liberty.num_rebels_from_bells_only,
            population,
            player.fathers
                .has[e_founding_father::simon_bolivar] );
    int const sons_of_liberty_integral_percent =
        compute_sons_of_liberty_integral_percent(
            sons_of_liberty_percent );
    int const tory_integral_percent =
        100 - sons_of_liberty_integral_percent;
    CHECK_GE( tory_integral_percent, 0 );
    CHECK_LE( tory_integral_percent, 100 );
    double const probability =
        clamp( ( difficulty_term +
                 double( tory_integral_percent ) / 4.0 ) /
                   100.0,
               0.0, 1.0 );
    if( !rand.bernoulli( probability ) ) continue;
    return &up_colony;
  }
  return nullptr;
}

vector<e_unit_type> generate_uprising_units( IRand& rand,
                                             int const count ) {
  vector<e_unit_type> res;
  res.reserve( count );
  for( int i = 0; i < count; ++i )
    res.push_back( rand.pick_from_weighted_values(
        config_revolution.uprising.unit_type_probabilities ) );
  return res;
}

vector<pair<e_unit_type, point>> distribute_uprising_units(
    IRand& rand, UprisingColony const& uprising_colony,
    vector<e_unit_type> const& unit_types ) {
  vector<pair<e_unit_type, point>> res;
  res.reserve( unit_types.size() );
  auto adjacent = uprising_colony.available_tiles_adjacent;
  auto beyond   = uprising_colony.available_tiles_beyond;
  rand.shuffle( adjacent );
  rand.shuffle( beyond );
  CHECK( !adjacent.empty() );
  vector<point> tiles;
  tiles.reserve( adjacent.size() * 2 + beyond.size() );
  for( point const p : adjacent ) tiles.push_back( p );
  for( point const p : adjacent ) tiles.push_back( p );
  for( point const p : beyond ) tiles.push_back( p );
  auto const cycled = rl::all( tiles ).cycle();
  for( auto it = cycled.begin();
       e_unit_type const type : unit_types )
    res.push_back( pair{ type, *it++ } );
  return res;
}

void deploy_uprising_units(
    SS& ss, Player const& ref_player, IMapUpdater& map_updater,
    UprisingColony const& uprising_colony,
    vector<pair<e_unit_type, point>> units ) {
  ColonyId const colony_id = uprising_colony.colony_id;
  Colony& colony           = ss.colonies.colony_for( colony_id );
  colony.had_tory_uprising = true;
  for( auto const& [type, tile] : units ) {
    UnitId const unit_id = create_unit_on_map_non_interactive(
        ss, map_updater, ref_player, type, tile );
    Unit& unit = ss.units.unit_for( unit_id );
    // Should already be cleared, but just to emphasize, since we
    // want these units to attack on the same turn that they
    // land, as in the OG. Note that this is different from when
    // normal REF troops land, which have to wait until the next
    // turn to attack.
    unit.new_turn( ref_player );
  }
}

wait<> show_uprising_msg(
    SSConst const& ss, IGui& gui,
    UprisingColony const& uprising_colony ) {
  ColonyId const colony_id = uprising_colony.colony_id;
  Colony const& colony     = ss.colonies.colony_for( colony_id );
  string const msg =
      format( "Tory uprising in [{}]!", colony.name );
  co_await gui.message_box( "{}", msg );
}

} // namespace rn
