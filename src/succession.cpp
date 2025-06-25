/****************************************************************
**succession.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-11.
*
* Description: Implements the War of Succession.
*
*****************************************************************/
#include "succession.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "igui.hpp"
#include "imap-updater.hpp"
#include "rebel-sentiment.hpp"
#include "sons-of-liberty.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "visibility.hpp"

// config
#include "config/nation.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/events.rds.hpp"
#include "ss/nation.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// C++ standard library
#include <ranges>

using namespace std;

namespace rg = std::ranges;

namespace rn {

namespace {

using ::gfx::point;
using ::refl::enum_count;
using ::refl::enum_map;
using ::refl::enum_values;

} // namespace

/****************************************************************
** War of Succession.
*****************************************************************/
bool should_do_war_of_succession( SSConst const& ss,
                                  Player const& player ) {
  // NOTE: there a corresponding config option, but we don't
  // check that directly because that is intended to be a default
  // value for the new-game setting that we actually check.
  if( ss.settings.game_setup_options.disable_war_of_succession )
    return false;
  // This human check is very important not only because the OG
  // does not trigger the war of succession in response to AI
  // player sentiment, but also because if this is an AI player
  // then we risk eliminating the player when it is their turn,
  // which is probably not a good thing.
  if( player.control != e_player_control::human ) return false;
  if( ss.events.war_of_succession_done ) return false;
  auto const [player_count, human_count] = [&] {
    int player_count = 0;
    int human_count  = 0;
    for( auto const& [player, other_player] :
         ss.players.players ) {
      if( !other_player.has_value() ) continue;
      if( other_player->control == e_player_control::withdrawn )
        continue;
      if( other_player->control != e_player_control::human &&
          other_player->revolution.status ==
              e_revolution_status::won )
        // AI player has been granted independence.
        continue;
      ++player_count;
      if( other_player->control == e_player_control::human )
        ++human_count;
    }
    return pair{ player_count, human_count };
  }();
  if( player_count < 4 ) return false;
  if( human_count != 1 )
    // If we only have AI players then we're not going to be
    // fighting a war of independence, so no need for the war of
    // succession. Likewise, if there are multiple human players,
    // it'd probably be strange to have a war of succession,
    // since it is already a non traditional mode of gameplay.
    return false;
  int const required_sentiment =
      required_rebel_sentiment_for_declaration( ss );
  if( player.revolution.rebel_sentiment < required_sentiment )
    return false;
  return true;
}

WarOfSuccessionNations select_players_for_war_of_succession(
    SSConst const& ss ) {
  vector<e_nation> ai_nations;
  ai_nations.reserve( enum_count<e_player> );
  enum_map<e_nation, int> size_metric;
  for( e_nation const nation : enum_values<e_nation> ) {
    e_player const player_type = colonist_player_for( nation );
    auto const& player         = ss.players.players[player_type];
    if( !player.has_value() ) continue;
    if( player->control == e_player_control::withdrawn )
      continue;
    if( player->control == e_player_control::human ) continue;
    ai_nations.push_back( nation );
    int const population =
        unit_count_for_rebel_sentiment( ss, player_type );
    int const colony_count =
        ss.colonies.for_player( player_type ).size();
    // This appears to be roughly what the OG does. There could
    // be further checks or other slight differences, but it
    // doesn't seem important to get it exactly the same, and
    // it's a bit tricky to determine empirically.
    size_metric[nation] = population + colony_count;
  }
  stable_sort( ai_nations.begin(), ai_nations.end(),
               [&]( e_nation const l, e_nation const r ) {
                 return size_metric[l] < size_metric[r];
               } );
  // The call to should_do_war_of_succession that we should have
  // done before calling this method should have ensured that
  // this won't happen.
  CHECK_GE( ssize( ai_nations ), 2 );
  e_nation const smallest        = ai_nations[0];
  e_nation const second_smallest = ai_nations[1];
  return WarOfSuccessionNations{ .withdraws = smallest,
                                 .receives  = second_smallest };
}

WarOfSuccessionPlan war_of_succession_plan(
    SSConst const& ss, WarOfSuccessionNations const& nations ) {
  WarOfSuccessionPlan plan;
  plan.nations = nations;

  unordered_map<UnitId, UnitState::euro const*> const& euros =
      ss.units.euro_all();
  for( auto const& [unit_id, p_state] : euros ) {
    Unit const& unit = p_state->unit;
    if( unit.player_type() !=
        colonist_player_for( nations.withdraws ) )
      continue;
    SWITCH( p_state->ownership ) {
      CASE( free ) { SHOULD_NOT_BE_HERE; }
      CASE( cargo ) {
        // Cargo units will get reassigned if/when what is
        // holding them gets reassigned (or they could be de-
        // stroyed if they are contained in a ship that is in
        // port, which do not get reassigned).
        break;
      }
      CASE( colony ) {
        // We don't reassign units working in colonies here be-
        // cause they will be reassigned along with the colony
        // further below.
        break;
      }
      CASE( dwelling ) {
        plan.reassign_units.push_back( unit_id );
        // This is so that the missionary cross color gets up-
        // dated on the dwelling.
        point const tile = ss.natives.coord_for( dwelling.id );
        plan.update_fog_squares.push_back( tile );
        break;
      }
      CASE( world ) {
        plan.reassign_units.push_back( unit_id );
        break;
      }
      CASE( harbor ) {
        SWITCH( harbor.port_status ) {
          CASE( in_port ) {
            plan.remove_units.push_back( unit_id );
            break;
          }
          CASE( inbound ) {
            plan.reassign_units.push_back( unit_id );
            break;
          }
          CASE( outbound ) {
            plan.reassign_units.push_back( unit_id );
            break;
          }
        }
        break;
      }
    }
  }

  for( auto const& [colony_id, colony] : ss.colonies.all() ) {
    if( colony.player !=
        colonist_player_for( nations.withdraws ) )
      continue;
    plan.reassign_colonies.push_back( colony_id );
    plan.update_fog_squares.push_back( colony.location );
  }

  rg::sort( plan.update_fog_squares,
            []( auto const l, auto const r ) {
              if( l.y != r.y ) return l.y < r.y;
              return l.x < r.x;
            } );
  auto const rg_erase = rg::unique( plan.update_fog_squares );
  plan.update_fog_squares.erase( rg_erase.begin(),
                                 rg_erase.end() );

  // Need determinism for unit tests.
  rg::sort( plan.remove_units );
  rg::sort( plan.reassign_units );
  rg::sort( plan.reassign_colonies );

  return plan;
}

void do_war_of_succession( SS& ss, TS& ts, Player const& player,
                           WarOfSuccessionPlan const& plan ) {
  CHECK_NEQ( player.type,
             colonist_player_for( plan.nations.withdraws ) );
  CHECK_NEQ( player.type,
             colonist_player_for( plan.nations.receives ) );
  destroy_units( ss, plan.remove_units );

  for( UnitId const unit_id : plan.reassign_units ) {
    // The unit could have been deleted if e.g. it was a unit in
    // the cargo of a ship in the harbor.
    if( !ss.units.exists( unit_id ) ) continue;
    Unit& unit = ss.units.unit_for( unit_id );
    change_unit_player(
        ss, ts, unit,
        colonist_player_for( plan.nations.receives ) );
  }

  for( ColonyId const colony_id : plan.reassign_colonies ) {
    Colony& colony = ss.colonies.colony_for( colony_id );
    change_colony_player(
        ss, ts, colony,
        colonist_player_for( plan.nations.receives ) );
    // The OG appears to reduce SoL to zero for the acquired
    // player. This is likely because then the merger would risk
    // causing a large bump to the total number of rebels in the
    // acquiring player and thus could risk immediately causing
    // them to be granted independence, which would lead to a
    // strange player experience. This way, the AI nations are no
    // further along in that process than they were before.
    reset_colony_sons_of_liberty( colony );
    // TODO: not sure yet how we'll be handling evolution of the
    // AI colonies, but this lowering of SoL to zero (under stan-
    // dard rules) could cause a drop in productivity and thus
    // starvation in the colony, so we may need to deal with that
    // here.
  }

  vector<Coord> const refresh_fogged = [&] {
    vector<Coord> res;
    res.reserve( plan.update_fog_squares.size() );
    VisibilityForPlayer const viz( ss, player.type );
    for( Coord const tile : plan.update_fog_squares )
      if( viz.visible( tile ) == e_tile_visibility::fogged )
        res.push_back( tile );
    return res;
  }();
  // This is not perfect because it will refresh the contents of
  // the entire tile, not limited to the player change. E.g. for
  // a colony it will not only update the player but will also
  // update the population and the fort type. But this makes
  // things simpler, and probably doesn't make much of a differ-
  // ence anyway.
  ts.map_updater().make_squares_visible( player.type,
                                         refresh_fogged );
  ts.map_updater().make_squares_fogged( player.type,
                                        refresh_fogged );

  ss.players
      .players[colonist_player_for( plan.nations.withdraws )]
      ->control = e_player_control::withdrawn;

  ss.events.war_of_succession_done = true;
}

wait<> do_war_of_succession_ui_seq(
    TS& ts, WarOfSuccessionPlan const& plan ) {
  // TODO: add more historical context to this message.
  string const msg = format(
      "The War of the Spanish Succession has come to an end in "
      "Europe! All property and territory owned by the [{}] has "
      "been ceded to the [{}].  As a result, the [{}] have "
      "withdrawn from the New World.",
      config_nation
          .players[colonist_player_for( plan.nations.withdraws )]
          .possessive_pre_declaration,
      config_nation
          .players[colonist_player_for( plan.nations.receives )]
          .possessive_pre_declaration,
      config_nation
          .players[colonist_player_for( plan.nations.withdraws )]
          .possessive_pre_declaration );
  co_await ts.gui.message_box( msg );
}

} // namespace rn
