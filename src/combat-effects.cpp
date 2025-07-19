/****************************************************************
**combat-effects.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-07-22.
*
* Description: Performs actions that need to happen when a unit
*              finishes a battle.
*
*****************************************************************/
#include "combat-effects.hpp"

// Rds.
#include "combat-effects-impl.rds.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colony-buildings.hpp"
#include "damaged.hpp"
#include "icombat.rds.hpp"
#include "ieuro-agent.hpp"
#include "map-search.hpp"
#include "revolution-status.hpp"
#include "society.hpp"
#include "tribe-arms.hpp"
#include "tribe.rds.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unit-ownership.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/nation.hpp"
#include "config/natives.hpp"
#include "config/text.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/natives.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** Helpers.
*****************************************************************/
void append_effects_msgs(
    UnitCombatEffectsMessages& to,
    UnitCombatEffectsMessages const& from ) {
  auto append = []( auto& to, auto const& from ) {
    to.insert( to.end(), from.begin(), from.end() );
  };
  append( to.for_owner, from.for_owner );
  append( to.for_other, from.for_other );
  append( to.for_both, from.for_both );
}

/****************************************************************
** Combat effects messages for individual attackers/defenders.
*****************************************************************/
UnitCombatEffectsMessages euro_unit_combat_effects_msg(
    SSConst const& ss, Player const& player, Unit const& unit,
    EuroUnitCombatOutcome const& outcome ) {
  UnitCombatEffectsMessages res;

  auto capture_unit = [&]( e_player new_player ) {
    string qualifier;
    switch( unit.type() ) {
      case e_unit_type::treasure: {
        int const worth =
            unit.composition()
                .inventory()[e_unit_inventory::gold];
        qualifier =
            fmt::format( " worth [{}{}]", worth,
                         config_text.special_chars.currency );
        break;
      }
      default:
        break;
    }
    res.for_both.push_back( fmt::format(
        "[{}] [{}]{} captured by the [{}]!",
        player_possessive( player ), unit.desc().name, qualifier,
        player_display_name( player_for_player_or_die(
            ss.players, new_player ) ) ) );
  };

  SWITCH( outcome ) {
    CASE( no_change ) { break; }
    CASE( destroyed ) {
      switch( unit.type() ) {
        case e_unit_type::damaged_artillery:
          res.for_both.push_back( fmt::format(
              "Damaged [{}] Artillery has been [destroyed].",
              player_possessive( player ) ) );
          break;
        default:
          // This will be scouts, pioneers, missionaries, and ar-
          // tillery.
          res.for_owner.push_back( fmt::format(
              "[{}] [{}] lost in battle.",
              player_possessive( player ), unit.desc().name ) );
          break;
      }
      break;
    }
    CASE( captured ) {
      capture_unit( captured.new_player );
      break;
    }
    CASE( captured_and_demoted ) {
      capture_unit( captured_and_demoted.new_player );
      string msg = "Unit demoted upon capture.";
      if( unit.type() == e_unit_type::veteran_colonist )
        msg = "Veteran status lost upon capture.";
      // This message only goes to the player that is capturing
      // the unit being demoted.
      res.for_other.push_back( msg );
      break;
    }
    CASE( demoted ) {
      auto& to_desc = unit_attr( demoted.to );
      switch( demoted.to.type() ) {
        case e_unit_type::damaged_artillery:
          res.for_both.push_back( fmt::format(
              "[{}] Artillery [damaged]. Further damage will "
              "destroy it.",
              player_possessive( player ) ) );
          break;
        default:
          string demoted_to;
          if( !to_desc.can_attack &&
              is_unit_a_colonist( to_desc.type ) )
            demoted_to = "colonist status";
          else
            demoted_to = fmt::format( "[{}]", to_desc.name );
          res.for_both.push_back( fmt::format(
              "[{}] [{}] routed! Unit demoted to {}.",
              player_possessive( player ), unit.desc().name,
              demoted_to ) );
          break;
      }
      break;
    }
    CASE( promoted ) {
      if( unit.desc().type == promoted.to.type() ) {
        CHECK_NEQ( unit.base_type(), promoted.to.base_type() );
        res.for_owner.push_back( fmt::format(
            "[{}] {}, acting as {}, has been promoted to [{}] "
            "for victory in combat!",
            player_possessive( player ),
            unit_attr( unit.base_type() ).name, unit.desc().name,
            unit_attr( promoted.to.base_type() ).name ) );
      } else {
        res.for_owner.push_back( fmt::format(
            "[{}] {} promoted to [{}] for victory in combat!",
            player_possessive( player ), unit.desc().name,
            unit_attr( promoted.to ).name ) );
      }
      break;
    }
  }

  return res;
}

static maybe<std::string> brave_promotion_message(
    e_tribe const tribe, e_native_unit_type const from,
    e_native_unit_type const to ) {
  CHECK_GT( to, from );
  string_view const tribe_name =
      config_natives.tribes[tribe].name_singular;
  string_view const from_name_plural =
      config_natives.unit_types[from].name_plural;
  auto const acquisition = [&]() -> e_brave_equipment {
    auto const& from_equip = config_natives.arms.equipment[from];
    auto const& to_equip   = config_natives.arms.equipment[to];
    // In the OG's rules only one of these can be obtained at a
    // time since there is no unit that can lose both muskets and
    // horses in one battle. Also, braves can't lose horses or
    // muskets in a battle, they just get destroyed.
    for( auto const eq : refl::enum_values<e_brave_equipment> )
      if( to_equip[eq] && !from_equip[eq] ) //
        return eq;
    // Config validation should ensure that this never happens
    // together with the check above that `to` > `from`.
    SHOULD_NOT_BE_HERE;
  }();
  return fmt::format(
      "[{}] {} have acquired [{}] upon victory in combat!",
      tribe_name, from_name_plural, acquisition );
}

UnitCombatEffectsMessages native_unit_combat_effects_msg(
    SSConst const& ss, NativeUnit const& unit,
    NativeUnitCombatOutcome const& outcome ) {
  UnitCombatEffectsMessages res;
  SWITCH( outcome ) {
    CASE( no_change ) { break; }
    CASE( destroyed ) { break; }
    CASE( promoted ) {
      maybe<string> const maybe_msg = brave_promotion_message(
          tribe_type_for_unit( ss, unit ), unit.type,
          promoted.to );
      if( maybe_msg.has_value() )
        res.for_both.push_back( *maybe_msg );
      break;
    }
  }
  return res;
}

std::string naval_battle_opponent_clause(
    SSConst const& ss, NavalBattleOpponent const& opponent ) {
  e_player player = {};
  string unit_part;
  SWITCH( opponent ) {
    CASE( colony ) {
      Colony const& opposing_colony =
          ss.colonies.colony_for( colony.id );
      maybe<e_colony_building> const fortification =
          building_for_slot( opposing_colony,
                             e_colony_building_slot::wall );
      CHECK(
          fortification.has_value() &&
              ( *fortification == e_colony_building::fort ||
                *fortification == e_colony_building::fortress ),
          "colony {} cannot attack a ship without a fort or "
          "fortress",
          colony.id );
      CHECK(
          opposing_colony.buildings[e_colony_building::fort] ||
          opposing_colony
              .buildings[e_colony_building::fortress] );
      player = opposing_colony.player;
      unit_part =
          config_colony.building_display_names[*fortification];
      break;
    }
    CASE( unit ) {
      Unit const& opponent_unit = ss.units.unit_for( unit.id );
      player                    = opponent_unit.player_type();
      unit_part                 = opponent_unit.desc().name;
      break;
    }
  }
  return fmt::format(
      "[{}] [{}]",
      player_possessive(
          player_for_player_or_die( ss.players, player ) ),
      unit_part );
}

UnitCombatEffectsMessages naval_unit_combat_effects_msg(
    SSConst const& ss, Unit const& unit,
    NavalBattleOpponent const& opponent,
    EuroNavalUnitCombatOutcome const& outcome ) {
  UnitCombatEffectsMessages res;

  auto add_units_lost = [&] {
    maybe<string> msg = units_lost_on_ship_message( unit );
    if( !msg.has_value() ) return;
    res.for_owner.push_back( std::move( *msg ) );
  };

  SWITCH( outcome ) {
    CASE( no_change ) { break; }
    CASE( moved ) { break; }
    CASE( damaged ) {
      res.for_both.push_back( ship_damaged_message(
          ss, unit.player_type(), unit.type(),
          e_ship_damaged_reason::battle, damaged.port ) );
      add_units_lost();
      break;
    }
    CASE( sunk ) {
      res.for_both.push_back( fmt::format(
          "[{}] [{}] sunk by {}.",
          player_possessive( player_for_player_or_die(
              ss.players, unit.player_type() ) ),
          unit.desc().name,
          naval_battle_opponent_clause( ss, opponent ) ) );
      add_units_lost();
      break;
    }
  }
  return res;
}

/****************************************************************
** Producing combat summaries.
*****************************************************************/
// The "unit" is the one whose player we are summarizing for.
string summarize_for_entity(
    SSConst const& ss, bool won, e_player const unit_player_type,
    Coord unit_coord, string_view unit_name,
    e_player const opponent_player_type, Coord opponent_coord,
    string_view opponent_name, string_view near_default ) {
  Player const& unit_player =
      player_for_player_or_die( ss.players, unit_player_type );
  Player const& opponent_player = player_for_player_or_die(
      ss.players, opponent_player_type );
  string_view const player_adj =
      player_possessive( unit_player );
  string_view const player_name =
      player_display_name( unit_player );
  string_view const opponent_player_adj =
      player_possessive( opponent_player );
  string_view const opponent_player_name =
      player_display_name( opponent_player );
  maybe<Colony const&> const closest_colony =
      find_close_explored_colony(
          ss, unit_player_type, unit_coord,
          /*max_distance=*/
          config_colony.search_dist_for_nearby_colony );
  string const colony_str =
      closest_colony.has_value()
          ? ( ( closest_colony->location == unit_coord ) ||
              ( closest_colony->location == opponent_coord ) )
                ? fmt::format( " in {}", closest_colony->name )
                : fmt::format( " near {}", closest_colony->name )
          : string( near_default );
  // E.g. "[English] Artillery defeats [French] near Roanoke!".
  return won ? fmt::format( "[{}] {} defeats [{}]{}!",
                            player_adj, unit_name,
                            opponent_player_name, colony_str )
             : fmt::format( "[{}] {} defeats [{}]{}!",
                            opponent_player_adj, opponent_name,
                            player_name, colony_str );
}

string summarize_for_euro_unit( SSConst const& ss, bool won,
                                auto const& unit_stats,
                                auto const& opponent_stats,
                                string_view near_default ) {
  Unit const& unit     = ss.units.unit_for( unit_stats.id );
  Unit const& opponent = ss.units.unit_for( opponent_stats.id );
  return summarize_for_entity(
      ss, won, unit.player_type(),
      coord_for_unit_multi_ownership_or_die( ss, unit.id() ),
      unit.desc().name, opponent.player_type(),
      coord_for_unit_multi_ownership_or_die( ss, opponent.id() ),
      opponent.desc().name, near_default );
}

CombatEffectsSummaries summarize_combat_outcome(
    SSConst const& ss, CombatEuroAttackEuro const& combat ) {
  static constexpr string_view kNearDefault =
      " in the wilderness";
  return {
    .attacker = summarize_for_euro_unit(
        ss, combat.winner == e_combat_winner::attacker,
        combat.attacker, combat.defender, kNearDefault ),
    .defender = summarize_for_euro_unit(
        ss, combat.winner == e_combat_winner::defender,
        combat.defender, combat.attacker, kNearDefault ),
  };
}

CombatEffectsSummaries summarize_combat_outcome(
    SSConst const& ss, CombatBraveAttackEuro const& combat ) {
  Unit const& defender = ss.units.unit_for( combat.defender.id );
  Player const& defending_player = player_for_player_or_die(
      ss.players, defender.player_type() );
  NativeUnit const& attacker =
      ss.units.unit_for( combat.attacker.id );
  e_tribe const tribe_type = tribe_type_for_unit( ss, attacker );
  string_view const tribe_name =
      config_natives.tribes[tribe_type].name_singular;
  string_view const player_adj =
      player_possessive( defending_player );
  string_view const euro_unit_name = defender.desc().name;
  string_view const native_unit_name =
      unit_attr( attacker.type ).name;
  Coord const defender_coord =
      coord_for_unit_multi_ownership_or_die( ss, defender.id() );
  maybe<Colony const&> const closest_colony =
      find_close_explored_colony(
          ss, defender.player_type(), defender_coord,
          /*max_distance=*/
          config_colony.search_dist_for_nearby_colony );
  CHECK( !closest_colony.has_value() ||
             ( closest_colony->location != defender_coord ),
         "this method should not be used when a brave is "
         "attacking a colony." );
  string const colony_str =
      closest_colony.has_value()
          ? fmt::format( " near {}", closest_colony->name )
          : " in the wilderness";
  switch( combat.winner ) {
    case e_combat_winner::attacker: {
      // Brave wins. Note that the below works with both
      // singular and plural unit names.
      return { .defender = fmt::format(
                   "[{}] ambush [{}] [{}]{}!", tribe_name,
                   player_adj, euro_unit_name, colony_str ) };
    }
    case e_combat_winner::defender: {
      // European wins.
      // "[English] Artillery defeats [Sioux] near Roanoke!".
      return { .defender = fmt::format(
                   "[{}] {} defeats [{}] {}{}!", player_adj,
                   euro_unit_name, tribe_name, native_unit_name,
                   colony_str ) };
    }
  }
}

CombatEffectsSummaries summarize_combat_outcome(
    SSConst const& ss, CombatEuroAttackBrave const& combat ) {
  Unit const& attacker = ss.units.unit_for( combat.attacker.id );
  Player const& attacking_player = player_for_player_or_die(
      ss.players, attacker.player_type() );
  NativeUnit const& defender =
      ss.units.unit_for( combat.defender.id );
  e_tribe const tribe_type = tribe_type_for_unit( ss, defender );
  string_view const tribe_name =
      config_natives.tribes[tribe_type].name_singular;
  string_view const player_adj =
      player_possessive( attacking_player );
  string_view const euro_unit_name = attacker.desc().name;
  string_view const native_unit_name =
      unit_attr( defender.type ).name;
  Coord const attacker_coord =
      coord_for_unit_multi_ownership_or_die( ss, attacker.id() );
  maybe<Colony const&> const closest_colony =
      find_close_explored_colony(
          ss, attacker.player_type(), attacker_coord,
          /*max_distance=*/
          config_colony.search_dist_for_nearby_colony );
  string const proximity_str =
      closest_colony.has_value()
          ? fmt::format( " near {}", closest_colony->name )
          : " in the wilderness";
  switch( combat.winner ) {
    case e_combat_winner::attacker: {
      return { .attacker = fmt::format(
                   "[{}] {} defeats [{}] {}{}!", player_adj,
                   euro_unit_name, tribe_name, native_unit_name,
                   proximity_str ) };
    }
    case e_combat_winner::defender: {
      return { .attacker = fmt::format(
                   "[{}] defeat [{}] [{}]{}!", tribe_name,
                   player_adj, euro_unit_name, proximity_str ) };
    }
  }
}

// This is for summarizing an attack against a unit defending a
// colony in the case where the colony is not destroyed.
CombatEffectsSummaries
summarize_non_destroying_combat_outcome_in_colony(
    SSConst const& ss, CombatBraveAttackColony const& combat,
    Colony const& colony ) {
  Unit const& defender = ss.units.unit_for( combat.defender.id );
  Player const& defending_player = player_for_player_or_die(
      ss.players, defender.player_type() );
  NativeUnit const& attacker =
      ss.units.unit_for( combat.attacker.id );
  e_tribe const tribe_type = tribe_type_for_unit( ss, attacker );
  string_view const tribe_name =
      config_natives.tribes[tribe_type].name_singular;
  string_view const player_adj =
      player_possessive( defending_player );
  string_view const euro_unit_name = defender.desc().name;
  switch( combat.winner ) {
    case e_combat_winner::attacker: {
      // Brave wins. Note that `ambush` works with both singular
      // and plural unit names.
      if( is_military_unit( defender.type() ) )
        return { .defender = fmt::format(
                     "[{}] ambush [{}] [{}] in [{}]!",
                     tribe_name, player_adj, euro_unit_name,
                     colony.name ) };
      else
        return { .defender = fmt::format(
                     "[{}] ambush [{}] colonists in [{}]! [{}] "
                     "lost while defending against the attack.",
                     tribe_name, player_adj, colony.name,
                     euro_unit_name ) };
    }
    case e_combat_winner::defender: {
      // European wins.
      //   [Inca] raiding party wiped out in Roanoke! Colonists
      //   celebrate!
      return { .defender =
                   fmt::format( "[{}] raiding party wiped out "
                                "in [{}]! Colonists celebrate!",
                                tribe_name, colony.name ) };
    }
  }
}

// This is for summarizing an attack against a unit defending a
// colony in the case where the colony is destroyed.
CombatEffectsSummaries summarize_colony_burn_combat_outcome(
    SSConst const& ss, CombatBraveAttackColony const& combat ) {
  CHECK( combat.colony_destroyed );
  // Sample:
  //   "[Souix] massacre [English] colonists at [Penobscot]!
  //    Colony burned to the ground! King demands explanation!"
  NativeUnit const& attacker =
      ss.units.unit_for( combat.attacker.id );
  e_tribe const tribe_type = tribe_type_for_unit( ss, attacker );
  string_view const tribe_name =
      config_natives.tribes[tribe_type].name_singular;
  Colony const& colony =
      ss.colonies.colony_for( combat.colony_id );
  Player const& colony_player =
      player_for_player_or_die( ss.players, colony.player );
  string_view const player_adj =
      player_possessive( colony_player );
  Player const& player =
      player_for_player_or_die( ss.players, colony.player );
  string const king_part =
      ( player.revolution.status <
        e_revolution_status::declared )
          ? " The King demands accountability!"
          : " The King laughs at such incompetent governance!";
  // Since colony burning is a big event, we will always show the
  // summary here.
  return { .defender = fmt::format(
               "[{}] massacre [{}] population in [{}]! Colony "
               "set ablaze and decimated!{}",
               tribe_name, player_adj, colony.name,
               king_part ) };
}

CombatEffectsSummaries summarize_combat_outcome(
    SSConst const& ss, CombatShipAttackShip const& combat ) {
  if( !combat.winner.has_value() )
    // Evade.
    return {};
  static constexpr string_view kNearDefault = " at sea";
  return {
    .attacker = summarize_for_euro_unit(
        ss, combat.winner == e_combat_winner::attacker,
        combat.attacker, combat.defender, kNearDefault ),
    .defender = summarize_for_euro_unit(
        ss, combat.winner == e_combat_winner::defender,
        combat.defender, combat.attacker, kNearDefault ),
  };
}

CombatEffectsSummaries summarize_combat_outcome(
    SSConst const& ss,
    CombatColonyArtilleryAttackShip const& combat ) {
  Colony const& colony =
      ss.colonies.colony_for( combat.attacker.id );
  Unit const& defender = ss.units.unit_for( combat.defender.id );
  Coord const defender_coord =
      ss.units.coord_for( defender.id() );
  // This should never be used because there will always be a
  // colony nearby, since the colony is the attacker.
  static constexpr string_view kNearDefault = "";
  static constexpr string_view kAttackerName =
      "coastal fortification";
  return { .attacker = summarize_for_entity(
               ss, combat.winner == e_combat_winner::attacker,
               colony.player, colony.location, kAttackerName,
               defender.player_type(), defender_coord,
               defender.desc().name, kNearDefault ),
           .defender = summarize_for_entity(
               ss, combat.winner == e_combat_winner::defender,
               defender.player_type(), defender_coord,
               defender.desc().name, colony.player,
               colony.location, kAttackerName, kNearDefault ) };
}

CombatEffectsSummaries summarize_combat_outcome(
    SSConst const& ss, CombatEuroAttackDwelling const& combat ) {
  Unit const& attacker = ss.units.unit_for( combat.attacker.id );
  Player const& attacking_player = player_for_player_or_die(
      ss.players, attacker.player_type() );
  DwellingId const dwelling_id = combat.defender.id;
  e_tribe const tribe_type =
      ss.natives.tribe_for( dwelling_id ).type;
  e_native_level const level =
      config_natives.tribes[tribe_type].level;
  string_view const dwelling_type_name =
      config_natives.dwelling_types[level].name_singular;
  string_view const tribe_name =
      config_natives.tribes[tribe_type].name_singular;
  string_view const tribe_adj =
      config_natives.tribes[tribe_type].name_possessive;
  string_view const player_adj =
      player_possessive( attacking_player );
  string_view const euro_unit_name = attacker.desc().name;
  Coord const attacker_coord =
      coord_for_unit_multi_ownership_or_die( ss, attacker.id() );
  maybe<Colony const&> const closest_colony =
      find_close_explored_colony(
          ss, attacker.player_type(), attacker_coord,
          /*max_distance=*/
          config_colony.search_dist_for_nearby_colony );
  string const proximity_str =
      closest_colony.has_value()
          ? fmt::format( " near {}", closest_colony->name )
          : fmt::format( " near a {} {}", tribe_adj,
                         dwelling_type_name );
  switch( combat.winner ) {
    case e_combat_winner::attacker: {
      return { .attacker = fmt::format(
                   "[{}] {} defeats [{}] Brave{}!", player_adj,
                   euro_unit_name, tribe_name, proximity_str ) };
    }
    case e_combat_winner::defender: {
      return { .attacker = fmt::format(
                   "[{}] defeat [{}] [{}]{}!", tribe_name,
                   player_adj, euro_unit_name, proximity_str ) };
    }
  }
}

CombatEffectsSummaries summarize_combat_outcome(
    SSConst const& ss,
    CombatEuroAttackUndefendedColony const& combat ) {
  if( combat.winner == e_combat_winner::attacker ) {
    // The colony has been overtaken. For the most part these
    // mes- sages are handled elsewhere.
    CHECK(
        combat.defender.outcome
            .holds<EuroColonyWorkerCombatOutcome::defeated>() );
    return {};
  }
  // Colony not taken over, so it's basically just a normal
  // battle outcome message.
  return summarize_combat_outcome(
      ss,
      CombatEuroAttackEuro{
        .winner   = combat.winner,
        .attacker = combat.attacker,
        .defender = {
          .id      = combat.defender.id,
          .outcome = EuroUnitCombatOutcome::no_change{} } } );
}

} // namespace

/****************************************************************
** Producing combat effects messages.
*****************************************************************/
CombatEffectsMessages combat_effects_msg(
    SSConst const& ss, CombatEuroAttackEuro const& combat ) {
  auto& attacker = ss.units.unit_for( combat.attacker.id );
  auto& defender = ss.units.unit_for( combat.defender.id );
  Player const& attacking_player = player_for_player_or_die(
      ss.players, attacker.player_type() );
  Player const& defending_player = player_for_player_or_die(
      ss.players, defender.player_type() );
  return { .summaries = summarize_combat_outcome( ss, combat ),
           .attacker  = euro_unit_combat_effects_msg(
               ss, attacking_player, attacker,
               combat.attacker.outcome ),
           .defender = euro_unit_combat_effects_msg(
               ss, defending_player, defender,
               combat.defender.outcome ) };
}

CombatEffectsMessages combat_effects_msg(
    SSConst const& ss, CombatShipAttackShip const& combat ) {
  auto& attacker = ss.units.unit_for( combat.attacker.id );
  auto& defender = ss.units.unit_for( combat.defender.id );
  Player const& attacking_player = player_for_player_or_die(
      ss.players, attacker.player_type() );
  Player const& defending_player = player_for_player_or_die(
      ss.players, defender.player_type() );
  CombatEffectsMessages res;
  if( !combat.winner.has_value() ) {
    // Defender evaded.
    string const evade_msg =
        fmt::format( "[{}] [{}] evades [{}] [{}].",
                     player_possessive( defending_player ),
                     defender.desc().name,
                     player_possessive( attacking_player ),
                     attacker.desc().name );
    res = { .summaries = summarize_combat_outcome( ss, combat ),
            .defender  = { .for_both = { evade_msg } } };
    return res;
  }
  res = { .summaries = summarize_combat_outcome( ss, combat ),
          .attacker  = naval_unit_combat_effects_msg(
              ss, attacker,
              NavalBattleOpponent::unit{ .id = defender.id() },
              combat.attacker.outcome ),
          .defender = naval_unit_combat_effects_msg(
              ss, defender,
              NavalBattleOpponent::unit{ .id = attacker.id() },
              combat.defender.outcome ) };
  for( auto const& [unit_id, affected_info] :
       combat.affected_defender_units ) {
    Unit const& affected_unit = ss.units.unit_for( unit_id );
    UnitCombatEffectsMessages const msgs =
        naval_unit_combat_effects_msg(
            ss, affected_unit,
            NavalBattleOpponent::unit{ .id = attacker.id() },
            affected_info.outcome );
    append_effects_msgs( res.defender, msgs );
  }
  return res;
}

CombatEffectsMessages combat_effects_msg(
    SSConst const& ss,
    CombatColonyArtilleryAttackShip const& combat ) {
  // In the OG there is no summary (as defined here) and no mes-
  // sage from the attacker (fort/fortress); there is only a
  // pre-battle message (that is not generated here), and pos-
  // sibly a message for the ship if it is damaged or sunk as a
  // result. Otherwise, there is nothing else.
  auto& defender = ss.units.unit_for( combat.defender.id );
  ColonyId const colony_id = combat.attacker.id;
  return { .summaries = summarize_combat_outcome( ss, combat ),
           .defender  = naval_unit_combat_effects_msg(
               ss, defender,
               NavalBattleOpponent::colony{ .id = colony_id },
               combat.defender.outcome ) };
}

CombatEffectsMessages combat_effects_msg(
    SSConst const& ss, CombatEuroAttackBrave const& combat ) {
  auto& attacker = ss.units.unit_for( combat.attacker.id );
  auto& defender = ss.units.unit_for( combat.defender.id );
  Player const& attacking_player = player_for_player_or_die(
      ss.players, attacker.player_type() );
  return { .summaries = summarize_combat_outcome( ss, combat ),
           .attacker  = euro_unit_combat_effects_msg(
               ss, attacking_player, attacker,
               combat.attacker.outcome ),
           .defender = native_unit_combat_effects_msg(
               ss, defender, combat.defender.outcome ) };
}

CombatEffectsMessages combat_effects_msg(
    SSConst const& ss, CombatBraveAttackEuro const& combat ) {
  auto& attacker = ss.units.unit_for( combat.attacker.id );
  auto& defender = ss.units.unit_for( combat.defender.id );
  Player const& defending_player = player_for_player_or_die(
      ss.players, defender.player_type() );
  return { .summaries = summarize_combat_outcome( ss, combat ),
           .attacker  = native_unit_combat_effects_msg(
               ss, attacker, combat.attacker.outcome ),
           .defender = euro_unit_combat_effects_msg(
               ss, defending_player, defender,
               combat.defender.outcome ) };
}

// The framework represented by this module doesn't really work
// well for the dwelling attack sequence as a whole, because
// there are multiple animations and may need to be interspersed
// with multiple messages, e.g. missionary released, convert pro-
// duced, treasure produced, in addition to the usual ones. So
// those messages are handled elsewhere, and this one just deals
// mainly with the usual effects on the european attacker unit.
CombatEffectsMessages combat_effects_msg(
    SSConst const& ss, CombatEuroAttackDwelling const& combat ) {
  auto& attacker = ss.units.unit_for( combat.attacker.id );
  Player const& attacking_player = player_for_player_or_die(
      ss.players, attacker.player_type() );
  return { .summaries = summarize_combat_outcome( ss, combat ),
           .attacker  = euro_unit_combat_effects_msg(
               ss, attacking_player, attacker,
               combat.attacker.outcome ) };
}

CombatEffectsMessages combat_effects_msg(
    SSConst const& ss, CombatBraveAttackColony const& combat ) {
  auto& defender = ss.units.unit_for( combat.defender.id );
  Player const& defending_player = player_for_player_or_die(
      ss.players, defender.player_type() );
  if( !combat.colony_destroyed ) {
    CombatEffectsMessages res{
      .summaries =
          summarize_non_destroying_combat_outcome_in_colony(
              ss, combat,
              ss.colonies.colony_for( combat.colony_id ) ),
      // There is no attacker message here because, as in the
      // OG, the brave always gets destroyed when attacking a
      // colony (whether it wins or loses), and there is no
      // message displayed for that specifically.
      .defender = euro_unit_combat_effects_msg(
          ss, defending_player, defender,
          combat.defender.outcome ) };
    if( !is_military_unit( defender.type() ) )
      // Kill all defender specific messages here because we want
      // the summary to be shown.
      res.defender = {};
    return res;
  }

  // Colony burned. In this case there is no attacker message for
  // the same reason as above, but also no defender message since
  // the colony burning pretty much overrides what the message
  // would otherwise been for the attacker, which would be some-
  // thing like "colonist destroyed."
  return { .summaries = summarize_colony_burn_combat_outcome(
               ss, combat ) };
}

CombatEffectsMessages combat_effects_msg(
    SSConst const& ss,
    CombatEuroAttackUndefendedColony const& combat ) {
  auto& attacker = ss.units.unit_for( combat.attacker.id );
  Player const& attacking_player = player_for_player_or_die(
      ss.players, attacker.player_type() );
  return { .summaries = summarize_combat_outcome( ss, combat ),
           .attacker  = euro_unit_combat_effects_msg(
               ss, attacking_player, attacker,
               combat.attacker.outcome ) };
}

/****************************************************************
** Showing combat effects messages.
*****************************************************************/
MixedCombatEffectsMessages mix_combat_effects_msgs(
    CombatEffectsMessages const& msg ) {
  auto flatten = []( vector<vector<string>> vv ) { // by copy.
    vector<string> res;
    res.reserve( 5 );
    for( vector<string>& v : vv )
      for( string& s : v )
        if( !s.empty() ) res.push_back( std::move( s ) );
    return res;
  };
  return {
    .summaries = msg.summaries,
    .attacker  = flatten(
        { msg.attacker.for_both, msg.attacker.for_owner,
           msg.defender.for_both, msg.defender.for_other } ),
    .defender = flatten(
        { msg.defender.for_both, msg.defender.for_owner,
          msg.attacker.for_both, msg.attacker.for_other } ) };
}

FilteredMixedCombatEffectsMessages filter_combat_effects_msgs(
    MixedCombatEffectsMessages const& msgs ) {
  FilteredMixedCombatEffectsMessages res;

  auto show_msg = []( vector<string>& to, string const& msg ) {
    if( msg.empty() ) return;
    to.push_back( msg );
  };

  auto show_msgs = [&]( vector<string>& to,
                        vector<string> const& msgs ) {
    for( string const& msg : msgs ) {
      // Empty messages should have been filtered out in the
      // mix_combat_effects_msgs function.
      CHECK( !msg.empty() );
      show_msg( to, msg );
    }
  };

  // In the OG the summary is only shown for the defender. This
  // is probably done so that, during the AI turns, attacks on
  // the human player that don't otherwise have messages associ-
  // ated with them will have at least one message for the player
  // to acknowledge before moving onto the next attack/move in
  // order give the player an opportunity to digest what just
  // happened, since the player didn't initiate it. For the at-
  // tacker, there really isn't a need for a summary because the
  // player will 1) see the animation to tell what the result
  // was, and also will have time to digest the result if only
  // when the next unit asks for orders. For similar reasons, we
  // only show the summary for the defender when there are no
  // other messages that get shown. This departs from the OG, but
  // that is probably OK because the OG displays non-summary mes-
  // sage before the animation happens, while we display all mes-
  // sages after the animation, and so putting the summary there
  // would serve no purpose if there are other messages that are
  // shown at the same time. Given all this, all of the attacker
  // summaries that are generated in this module are pointless,
  // but we generate them anyway just in case we find a reason to
  // enable them in the future.

  show_msgs( res.attacker, msgs.attacker );
  /* No summary for attacker, even if it has no msgs. */

  show_msgs( res.defender, msgs.defender );
  if( msgs.defender.empty() )
    show_msg( res.defender, msgs.summaries.defender );

  return res;
}

/****************************************************************
** Performing combat effects.
*****************************************************************/
void perform_euro_unit_combat_effects(
    SS& ss, TS& ts, Unit& unit,
    EuroUnitCombatOutcome const& outcome ) {
  auto capture_unit = [&]( e_player new_player,
                           Coord new_coord ) {
    // We need to use this special function to do the nation
    // change and the move at the same time. If we did the move
    // first then there would be an intermediate state where we'd
    // have multiple units of different players on the same
    // square, which we don't want. On the other hand, if we were
    // to do the the player change first then it would cause the
    // map squares around the captured unit to be revealed to the
    // capturing player (since the captured unit is still on the
    // original square when the player change happens) which ends
    // up looking visually surprising and confusing.
    change_unit_player_and_move( ss, ts, unit, new_player,
                                 new_coord );
    // This is so that the captured unit won't ask for orders
    // in the same turn that it is captured.
    unit.forfeight_mv_points();
    unit.clear_orders();
  };

  SWITCH( outcome ) {
    CASE( no_change ) { break; }
    CASE( destroyed ) {
      UnitOwnershipChanger( ss, unit.id() ).destroy();
      break;
    }
    CASE( captured ) {
      capture_unit( captured.new_player, captured.new_coord );
      break;
    }
    CASE( captured_and_demoted ) {
      capture_unit( captured_and_demoted.new_player,
                    captured_and_demoted.new_coord );
      change_unit_type( ss, ts, unit, captured_and_demoted.to );
      break;
    }
    CASE( demoted ) {
      // If the unit is being demoted to a non military unit and
      // they are fortified and they are in a colony then for a
      // good UX we should unfortify them.
      //
      // This is because a fortified non-military unit is never
      // chosen as a defender of a colony, so unfortifying the
      // unit will have it ask for orders because the player will
      // likely want to assign that unit to have another job (or
      // they e.g. give it muskets/horses again, which will also
      // unfortify it).
      //
      // However, if the unit is not in a colony then we should
      // not unfortify it, because non-military units in the open
      // do get a fortification bonus when defending. At least
      // that is what the combat analysis states; it is a bit
      // more complicated than this because there are other
      // penalties assigned to the unit when it is not armed, but
      // that is a separate matter.
      change_unit_type( ss, ts, unit, demoted.to );
      if( auto const p = ss.units.maybe_coord_for( unit.id() );
          p.has_value() &&
          ss.colonies.maybe_from_coord( *p ).has_value() ) {
        // The defending unit is at the gate of a colony.
        bool const fortified =
            unit.orders().holds<unit_orders::fortified>() ||
            unit.orders().holds<unit_orders::fortifying>();
        if( fortified && !is_military_unit( unit.type() ) )
          unit.clear_orders();
      }
      break;
    }
    CASE( promoted ) {
      change_unit_type( ss, ts, unit, promoted.to );
      break;
    }
  }
}

void perform_native_unit_combat_effects(
    SS& ss, NativeUnit& unit,
    NativeUnitCombatOutcome const& outcome ) {
  SWITCH( outcome ) {
    CASE( no_change ) { break; }
    CASE( destroyed ) {
      Tribe& tribe = tribe_for_unit( ss, unit );
      if( destroyed.tribe_retains_horses )
        retain_horses_from_destroyed_brave( ss, tribe );
      if( destroyed.tribe_retains_muskets )
        retain_muskets_from_destroyed_brave( tribe );
      ss.units.destroy_unit( unit.id );
      break;
    }
    CASE( promoted ) {
      unit.type = promoted.to;
      // In the OG, although a brave can gain both muskets and/or
      // horses when attacking and winning, the tribe's stock-
      // piles only increase for horses.
      if( promoted.tribe_gains_horse_herd )
        gain_horses_from_winning_combat(
            tribe_for_unit( ss, unit ) );
      break;
    }
  }
}

void perform_naval_unit_combat_effects(
    SS& ss, TS& ts, Unit& unit, UnitId opponent_id,
    EuroNavalUnitCombatOutcome const& outcome ) {
  SWITCH( outcome ) {
    CASE( no_change ) { break; }
    CASE( moved ) {
      // This ideally would be run interactively because e.g. the
      // ship might discover the pacific ocean or another nation
      // upon moving. However, that might be tricky to get right
      // because then we sit waiting for the message box before
      // the other unit has had its outcome performed. So what we
      // do is we just run this non-interactively, then after
      // both ships have their outcomes performed, we will
      // re-place the ship on the square interactively in order
      // to run any interactive routines that are necessary.
      UnitOwnershipChanger( ss, unit.id() )
          .change_to_map_non_interactive( ts.map_updater(),
                                          moved.to );
      break;
    }
    CASE( damaged ) {
      move_damaged_ship_for_repair( ss, ts.map_updater(), unit,
                                    damaged.port );
      break;
    }
    CASE( sunk ) {
      // This should always exist because, if we are here, then
      // this ship has been sunk, which means that the opponent
      // ship should not have been sunk and thus should exist.
      CHECK( ss.units.exists( opponent_id ) );
      UnitOwnershipChanger( ss, unit.id() ).destroy();
      break;
    }
  }
}

void perform_naval_affected_unit_combat_effects(
    SS& ss, TS& ts, UnitId opponent_id,
    AffectedNavalDefender const& affected ) {
  Unit& unit = ss.units.unit_for( affected.id );
  SWITCH( affected.outcome ) {
    CASE( damaged ) {
      return perform_naval_unit_combat_effects(
          ss, ts, unit, opponent_id, affected.outcome );
    }
    CASE( moved ) { SHOULD_NOT_BE_HERE; }
    CASE( no_change ) { SHOULD_NOT_BE_HERE; }
    CASE( sunk ) {
      return perform_naval_unit_combat_effects(
          ss, ts, unit, opponent_id, affected.outcome );
    }
  }
}

} // namespace rn
