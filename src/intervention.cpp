/****************************************************************
**intervention.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-11.
*
* Description: Implements the intervention force.
*
*****************************************************************/
#include "intervention.hpp"

// Revolution Now
#include "anim-builders.hpp"
#include "colony-mgr.hpp"
#include "connectivity.hpp"
#include "igui.hpp"
#include "irand.hpp"
#include "land-view.hpp"
#include "plane-stack.hpp"
#include "society.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unit-ownership.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/revolution.rds.hpp"

// ss
#include "revolution.rds.hpp"
#include "ss/colonies.hpp"
#include "ss/colony.rds.hpp"
#include "ss/nation.hpp"
#include "ss/player.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

// C++ standard library
#include <ranges>

using namespace std;

namespace rg = std::ranges;

namespace rn {

namespace {

using ::base::maybe;
using ::base::nothing;
using ::gfx::point;
using ::refl::enum_count;
using ::refl::enum_map;
using ::refl::enum_values;

} // namespace

/****************************************************************
** Intervention Force.
*****************************************************************/
int bells_required_for_intervention(
    SettingsState const& settings ) {
  return config_revolution.intervention_forces
      .bells_needed[settings.game_setup_options.difficulty];
}

// NOTE: the return value of this function cannot depend on game
// state or randomness... it must be deterministic because it is
// called multiple times (without storing the result) expecting
// that it will always yield the same output for the same input.
e_nation select_nation_for_intervention(
    e_nation const for_nation ) {
  using enum e_nation;
  // Always give preference to the French if they are available
  // since it reflects history.
  //
  // NOTE: it is not required that the player actually exist here
  // in order to provide the intervention force, since it is re-
  // ally the old world that sends the intervention forces. Also,
  // we do want there to be an intervention force when there is
  // only one player in the game. It also doesn't matter whether
  // the intervening player is human or AI, for similar reasons.
  switch( for_nation ) {
    case english:
      return french;
    case french:
      return spanish;
    case spanish:
      return french;
    case dutch:
      return french;
  }
}

bool should_trigger_intervention( SSConst const& ss,
                                  Player const& player ) {
  using enum e_revolution_status;
  if( player.revolution.status < declared ) return false;
  if( player.revolution.status >= won ) return false;
  if( player.revolution.intervention_force_deployed )
    return false;
  if( player.bells <
      bells_required_for_intervention( ss.settings ) )
    return false;
  return true;
}

void trigger_intervention( Player& player ) {
  player.revolution.intervention_force_deployed = true;
  player.bells                                  = 0;
}

maybe<InterventionLandUnits> pick_forces_to_deploy(
    Player const& player ) {
  InterventionLandUnits taken;
  InterventionForce available =
      player.revolution.intervention_force;
  auto const total_taken = [&] {
    // Land units we've taken so far.
    return taken.continental_army + taken.continental_cavalry +
           taken.artillery;
  };
  auto const total_available = [&] {
    // Land units still available to be taken.
    return available.continental_army +
           available.continental_cavalry + available.artillery;
  };
  if( available.man_o_war == 0 ) return nothing;
  taken.continental_army =
      std::min( 2, available.continental_army );
  taken.continental_cavalry =
      std::min( 2, available.continental_cavalry );
  taken.artillery = std::min( 2, available.artillery );
  available.continental_army -= taken.continental_army;
  available.continental_cavalry -= taken.continental_cavalry;
  available.artillery -= taken.artillery;
  array<pair<int*, int*>, 3> arr{
    pair{ &available.continental_army, &taken.continental_army },
    pair{ &available.continental_cavalry,
          &taken.continental_cavalry },
    pair{ &available.artillery, &taken.artillery },
  };
  while( total_taken() < 6 && total_available() > 0 ) {
    // Sort largest to smallest.
    rg::stable_sort( arr, []( auto const& l, auto const& r ) {
      return *r.first < *l.first;
    } );
    auto const& [p_available, p_taken] = arr[0];
    CHECK( p_available );
    CHECK( p_taken );
    CHECK_GT( *p_available, 0 );
    --( *p_available );
    ++( *p_taken );
  }
  // Even if total_taken() is zero at this point we will
  // return a value because we want the ship to be delivered.
  return taken;
}

maybe<InterventionDeployTile> find_intervention_deploy_tile(
    SSConst const& ss, IRand& rand,
    TerrainConnectivity const& connectivity,
    Player const& player ) {
  maybe<InterventionDeployTile> res;
  vector<ColonyId> colonies =
      ss.colonies.for_player( player.type );
  rg::sort( colonies );
  vector<e_direction> directions(
      enum_values<e_direction>.begin(),
      enum_values<e_direction>.end() );
  rand.shuffle( directions );
  rand.shuffle( colonies );
  for( ColonyId const colony_id : colonies ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    for( e_direction const d : directions ) {
      point const moved = colony.location.moved( d );
      if( !ss.terrain.square_exists( moved ) ) continue;
      if( ss.terrain.square_at( moved ).surface !=
          e_surface::water )
        continue;
      if( auto const society = society_on_square( ss, moved );
          society.has_value() ) {
        SWITCH( *society ) {
          CASE( european ) {
            // Keep in mind that when the intervention force is
            // deployed, all other european nations' units will
            // have been removed, so in practice this is just
            // testing if there is an REF ship in the way, in
            // which case the square will be avoided (confirmed
            // this is the behavior in OG). If this means that
            // there are no available tiles, then the interven-
            // tion force does not land this turn.
            if( european.player != player.type ) continue;
            break;
          }
          CASE( native ) {
            // Should never happen since this is a water square,
            // but let's be defensive against strange mods.
            continue;
          }
        }
      }
      if( !water_square_has_ocean_access( connectivity, moved ) )
        continue;
      res = InterventionDeployTile{ .tile      = moved,
                                    .colony_id = colony_id };
      return res;
    }
  }
  return res;
}

UnitId deploy_intervention_forces(
    SS& ss, TS& ts, InterventionDeployTile const& location,
    InterventionLandUnits const& forces ) {
  Colony const& colony =
      ss.colonies.colony_for( location.colony_id );
  UNWRAP_CHECK_T( Player & player,
                  ss.players.players[colony.player] );
  CHECK_GT( player.revolution.intervention_force.man_o_war, 0 );
  // Use the non-interactive version because it is unlikely that
  // any interactive stuff would need to be done in this situa-
  // tion. Technically the ship could discover the pacific ocean,
  // but still it would be strange to the player to see that sud-
  // denly in response to an intervention ship that just appears
  // out of nowhere (and before the message describing what the
  // ship is). So we will re-place the ship later.
  UnitId const ship_id = create_unit_on_map_non_interactive(
      ss, ts, player, e_unit_type::man_o_war, location.tile );
  auto const add_cargo_unit = [&]( e_unit_type const type ) {
    UnitId const held =
        create_free_unit( ss.units, player, type );
    UnitOwnershipChanger( ss, held )
        .change_to_cargo( ship_id, /*starting_slot=*/0 );
  };
  for( int i = 0; i < forces.continental_army; ++i )
    add_cargo_unit( e_unit_type::continental_army );
  for( int i = 0; i < forces.continental_cavalry; ++i )
    add_cargo_unit( e_unit_type::continental_cavalry );
  for( int i = 0; i < forces.artillery; ++i )
    add_cargo_unit( e_unit_type::artillery );
  player.revolution.intervention_force.continental_army -=
      forces.continental_army;
  player.revolution.intervention_force.continental_cavalry -=
      forces.continental_cavalry;
  player.revolution.intervention_force.artillery -=
      forces.artillery;
  player.revolution.intervention_force.man_o_war -= 1;
  CHECK_GE(
      player.revolution.intervention_force.continental_army, 0 );
  CHECK_GE(
      player.revolution.intervention_force.continental_cavalry,
      0 );
  CHECK_GE( player.revolution.intervention_force.artillery, 0 );
  CHECK_GE( player.revolution.intervention_force.man_o_war, 0 );
  return ship_id;
}

wait<> intervention_forces_intro_ui_seq(
    SSConst const& ss, IGui& gui, e_nation const receiving,
    e_nation const intervening ) {
  string const& receiver_name_possessive =
      config_nation.players[colonial_player_for( receiving )]
          .possessive_pre_declaration;
  string const& intervener_name_possessive =
      config_nation.players[colonial_player_for( intervening )]
          .possessive_pre_declaration;
  co_await gui.message_box(
      "The [{}] are considering intervening in the war against "
      "the {} Crown, your Excellency.  They will do so if we "
      "can generate [{}] liberty bells during the war.",
      intervener_name_possessive, receiver_name_possessive,
      config_revolution.intervention_forces.bells_needed
          [ss.settings.game_setup_options.difficulty] );
}

wait<> intervention_forces_triggered_ui_seq(
    SSConst const& ss, IGui& gui, e_nation const receiving,
    e_nation const intervening ) {
  auto const largest_colony = [&]() -> maybe<Colony const&> {
    vector<ColonyId> colonies = ss.colonies.for_player(
        colonial_player_for( receiving ) );
    rg::sort(
        colonies, [&]( ColonyId const l, ColonyId const r ) {
          Colony const& l_colony = ss.colonies.colony_for( l );
          Colony const& r_colony = ss.colonies.colony_for( r );
          return colony_population( l_colony ) <
                 colony_population( r_colony );
        } );
    if( colonies.empty() ) return nothing;
    return ss.colonies.colony_for( colonies.back() );
  }();
  if( !largest_colony.has_value() )
    // Player has no colonies... this is not likely to happen
    // in a normal game, but technically could, because the
    // player could end up in a state where independence is de-
    // clared and they have no colonies by the time the inter-
    // vention force is triggered (especially when initiated
    // via cheat mode).
    co_return;
  Colony const& colony = *largest_colony;
  string const& intervener_name =
      config_nation.nations[intervening].country_name;
  string const& receiver_name =
      config_nation.nations[receiving].country_name;
  string const& intervener_name_possessive =
      config_nation.players[colonial_player_for( intervening )]
          .possessive_pre_declaration;
  co_await gui.message_box(
      "[{}] declares war on {} and joins the War of "
      "Independence on the side of the Rebels! {} Generals meet "
      "in {} for strategic discussions. {} Navy plans to "
      "bombard Tory strongholds in the New World.",
      intervener_name, receiver_name, intervener_name_possessive,
      colony.name, intervener_name_possessive );
}

wait<> intervention_forces_deployed_ui_seq(
    TS& ts, Colony const& colony, e_nation const intervening ) {
  co_await ts.planes.get()
      .get_bottom<ILandViewPlane>()
      .ensure_visible( colony.location );
  string const& intervener_name =
      config_nation.players[colonial_player_for( intervening )]
          .possessive_pre_declaration;
  co_await ts.gui.message_box(
      "{} intervention forces arrive in [{}].  {} General joins "
      "forces with the Rebels.",
      intervener_name, colony.name, intervener_name );
}

wait<> animate_move_intervention_units_into_colony(
    SS& ss, TS& ts, UnitId const ship_id,
    Colony const& colony ) {
  point const ship_tile = ss.units.coord_for( ship_id );
  UNWRAP_CHECK_T( e_direction const d,
                  ship_tile.direction_to( colony.location ) );
  AnimationSequence const seq =
      anim_seq_for_offboard_intervention_force( ss, ship_id, d );
  co_await ts.planes.get().get_bottom<ILandViewPlane>().animate(
      seq );
}

void move_intervention_units_into_colony(
    SS& ss, TS& ts, UnitId const ship_id,
    Colony const& colony ) {
  Unit const& ship       = ss.units.unit_for( ship_id );
  auto const cargo_units = ship.cargo().units();
  for( UnitId const unit_id : cargo_units ) {
    UnitOwnershipChanger( ss, unit_id )
        .change_to_map_non_interactive( ts, colony.location );
    ss.units.unit_for( unit_id ).clear_orders();
  }
}

} // namespace rn
