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

// Revolution Now
#include "co-wait.hpp"
#include "damaged.hpp"
#include "icombat.rds.hpp"
#include "ieuro-mind.hpp"
#include "society.hpp"
#include "tribe-mgr.hpp"
#include "tribe.rds.hpp"
#include "unit-mgr.hpp"

// config
#include "config/nation.hpp"
#include "config/natives.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

using namespace std;

namespace rn {

namespace {

string concat_sentences(
    vector<vector<string>> const& msg_groups ) {
  string res;
  for( vector<string> const& msg_group : msg_groups ) {
    for( string const& msg : msg_group ) {
      if( msg.empty() ) continue;
      if( !res.empty() ) res += ' ';
      res += msg;
    }
  }
  return res;
}

} // namespace

/****************************************************************
** Producing combat effects messages.
*****************************************************************/
CombatEffectsMessage euro_unit_combat_effects_msg(
    Unit const& unit, EuroUnitCombatOutcome const& outcome ) {
  CombatEffectsMessage res;

  auto capture_unit = [&]( e_nation new_nation ) {
    res.for_both.push_back( fmt::format(
        "{} [{}] captured by the [{}]!",
        nation_obj( unit.nation() ).adjective, unit.desc().name,
        nation_obj( new_nation ).display_name ) );
  };

  SWITCH( outcome ) {
    CASE( no_change ) { break; }
    CASE( destroyed ) {
      // This will be scouts, pioneers, missionaries, and ar-
      // tillery.
      res.for_owner.push_back(
          fmt::format( "{} [{}] has been lost in battle!",
                       nation_obj( unit.nation() ).adjective,
                       unit.desc().name ) );
      break;
    }
    CASE( captured ) {
      capture_unit( captured.new_nation );
      break;
    }
    CASE( captured_and_demoted ) {
      capture_unit( captured_and_demoted.new_nation );
      string msg = "Unit demoted upon capture!";
      if( unit.type() == e_unit_type::veteran_colonist )
        msg = "Veteran status lost upon capture!";
      // This message only goes to the player that is capturing
      // the unit being demoted.
      res.for_other.push_back( msg );
      break;
    }
    CASE( demoted ) { break; }
    CASE( promoted ) {
      // TODO: make this message more specific like in the OG.
      res.for_owner.push_back(
          "Unit promoted for victory in combat!" );
      break;
    }
    END_CASES;
  }

  return res;
}

static maybe<std::string> brave_promotion_message(
    e_tribe tribe, e_native_unit_type from,
    e_native_unit_type to ) {
  string_view const tribe_name =
      config_natives.tribes[tribe].name_singular;
  string_view const from_name =
      config_natives.unit_types[from].name;
  using E                = e_native_unit_type;
  auto       p           = pair{ from, to };
  auto const acquisition = [&]() -> maybe<string> {
    if( p == pair{ E::brave, E::armed_brave } )
      return "muskets";
    else if( p == pair{ E::brave, E::mounted_brave } )
      return "horses";
    else if( p == pair{ E::armed_brave, E::mounted_warrior } )
      return "horses";
    else if( p == pair{ E::mounted_brave, E::mounted_warrior } )
      return "muskets";
    else
      return nothing;
  }();
  if( !acquisition.has_value() ) return nothing;
  return fmt::format(
      "[{}] {} has acquired [{}] upon victory in combat!",
      tribe_name, from_name, acquisition );
}

CombatEffectsMessage native_unit_combat_effects_msg(
    SSConst const& ss, NativeUnit const& unit,
    NativeUnitCombatOutcome const& outcome ) {
  CombatEffectsMessage res;
  SWITCH( outcome ) {
    CASE( no_change ) { break; }
    CASE( destroyed ) { break; }
    CASE( promoted ) {
      maybe<string> const maybe_msg = brave_promotion_message(
          tribe_for_unit( ss, unit ), unit.type, promoted.to );
      if( maybe_msg.has_value() )
        res.for_both.push_back( *maybe_msg );
      break;
    }
    END_CASES;
  }
  return res;
}

CombatEffectsMessage naval_unit_combat_effects_msg(
    SSConst const& ss, Unit const& unit, Unit const& opponent,
    EuroNavalUnitCombatOutcome const& outcome ) {
  CombatEffectsMessage res;

  auto add_units_lost = [&] {
    maybe<string> msg = units_lost_on_ship_message( unit );
    if( !msg.has_value() ) return;
    res.for_owner.push_back( std::move( *msg ) );
  };

  SWITCH( outcome ) {
    CASE( no_change ) { break; }
    CASE( moved ) { break; }
    CASE( damaged ) {
      res.for_both.push_back(
          ship_damaged_message( ss, unit, damaged.port ) );
      add_units_lost();
      break;
    }
    CASE( sunk ) {
      res.for_both.push_back(
          fmt::format( "{} [{}] sunk by [{}] {}.",
                       nation_obj( unit.nation() ).adjective,
                       unit.desc().name,
                       nation_obj( opponent.nation() ).adjective,
                       opponent.desc().name ) );
      add_units_lost();
      break;
    }
    END_CASES;
  }
  return res;
}

/****************************************************************
** Performing combat effects.
*****************************************************************/
void perform_euro_unit_combat_effects(
    SS& ss, TS& ts, Unit& unit,
    EuroUnitCombatOutcome const& outcome ) {
  auto capture_unit = [&]( e_nation new_nation,
                           Coord    new_coord ) {
    change_unit_nation( ss, ts, unit, new_nation );
    unit_ownership_change_non_interactive(
        ss, unit.id(),
        EuroUnitOwnershipChangeTo::world{
            .ts = &ts, .target = new_coord } );
    // This is so that the captured unit won't ask for orders
    // in the same turn that it is captured.
    unit.forfeight_mv_points();
    unit.clear_orders();
  };

  SWITCH( outcome ) {
    CASE( no_change ) { break; }
    CASE( destroyed ) {
      // This will be scouts, pioneers, missionaries, and ar-
      // tillery.
      destroy_unit( ss, unit.id() );
      break;
    }
    CASE( captured ) {
      capture_unit( captured.new_nation, captured.new_coord );
      break;
    }
    CASE( captured_and_demoted ) {
      capture_unit( captured_and_demoted.new_nation,
                    captured_and_demoted.new_coord );
      change_unit_type( ss, ts, unit, captured_and_demoted.to );
      break;
    }
    CASE( demoted ) {
      change_unit_type( ss, ts, unit, demoted.to );
      break;
    }
    CASE( promoted ) {
      change_unit_type( ss, ts, unit, promoted.to );
      break;
    }
    END_CASES;
  }
}

void perform_native_unit_combat_effects(
    SS& ss, NativeUnit& unit,
    NativeUnitCombatOutcome const& outcome ) {
  SWITCH( outcome ) {
    CASE( no_change ) { break; }
    CASE( destroyed ) {
      Tribe& tribe =
          ss.natives.tribe_for( tribe_for_unit( ss, unit ) );
      if( destroyed.tribe_retains_horses )
        tribe_take_horses_from_destroyed_brave( tribe );
      if( destroyed.tribe_retains_muskets )
        tribe_take_muskets_from_destroyed_brave( tribe );
      ss.units.destroy_unit( unit.id );
      break;
    }
    CASE( promoted ) {
      unit.type = promoted.to;
      break;
    }
    END_CASES;
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
      unit_ownership_change_non_interactive(
          ss, unit.id(),
          EuroUnitOwnershipChangeTo::world{
              .ts = &ts, .target = moved.to } );
      break;
    }
    CASE( damaged ) {
      move_damaged_ship_for_repair( ss, ts, unit, damaged.port );
      break;
    }
    CASE( sunk ) {
      // This should always exist because, if we are here, then
      // this ship has been sunk, which means that the opponent
      // ship should not have been sunk and thus should exist.
      CHECK( ss.units.exists( opponent_id ) );
      destroy_unit( ss, unit.id() );
      break;
    }
    END_CASES;
  }
}

/****************************************************************
** Showing combat effects messages.
*****************************************************************/
wait<> show_combat_effects_messages_euro_euro(
    EuroCombatEffectsMessage const& attacker,
    EuroCombatEffectsMessage const& defender ) {
  string const attacker_msg = concat_sentences(
      { attacker.msg.for_both, attacker.msg.for_owner,
        defender.msg.for_both, defender.msg.for_other } );
  if( !attacker_msg.empty() )
    co_await attacker.mind.message_box( attacker_msg );
  string const defender_msg = concat_sentences(
      { defender.msg.for_both, defender.msg.for_owner,
        attacker.msg.for_both, attacker.msg.for_other } );
  if( !defender_msg.empty() )
    co_await defender.mind.message_box( defender_msg );
}

wait<> show_combat_effects_messages_euro_attacker_only(
    EuroCombatEffectsMessage const& attacker ) {
  string const attacker_msg = concat_sentences(
      { attacker.msg.for_both, attacker.msg.for_owner } );
  if( !attacker_msg.empty() )
    co_await attacker.mind.message_box( attacker_msg );
}

wait<> show_combat_effects_messages_euro_native(
    EuroCombatEffectsMessage const&   euro,
    NativeCombatEffectsMessage const& native ) {
  string const attacker_msg = concat_sentences(
      { euro.msg.for_both, euro.msg.for_owner,
        native.msg.for_both, native.msg.for_other } );
  if( !attacker_msg.empty() )
    co_await euro.mind.message_box( attacker_msg );
}

} // namespace rn
