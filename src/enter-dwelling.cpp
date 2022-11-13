/****************************************************************
**enter-dwelling.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-06.
*
* Description: Drives the sequence of events that happen when a
*              unit attempts to enter a native dwelling.
*
*****************************************************************/
#include "enter-dwelling.hpp"

// Revolution Now
#include "alarm.hpp"
#include "co-wait.hpp"
#include "igui.hpp"
#include "promotion.hpp"
#include "ts.hpp"

// config
#include "config/natives.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/tribe.rds.hpp"
#include "ss/unit.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/string.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

e_dwelling_interaction_category dwelling_interaction_category(
    e_unit_type type ) {
  switch( type ) {
    case e_unit_type::soldier:
    case e_unit_type::dragoon:
    case e_unit_type::veteran_soldier:
    case e_unit_type::veteran_dragoon:
    case e_unit_type::continental_army:
    case e_unit_type::continental_cavalry:
    case e_unit_type::regular:
    case e_unit_type::cavalry:
    case e_unit_type::artillery:
    case e_unit_type::damaged_artillery:
      return e_dwelling_interaction_category::military;

    case e_unit_type::missionary:
    case e_unit_type::jesuit_missionary:
      return e_dwelling_interaction_category::missionary;

    case e_unit_type::scout:
    case e_unit_type::seasoned_scout:
      return e_dwelling_interaction_category::scout;

    case e_unit_type::petty_criminal:
    case e_unit_type::indentured_servant:
    case e_unit_type::free_colonist:
    case e_unit_type::native_convert:
    case e_unit_type::expert_farmer:
    case e_unit_type::expert_fisherman:
    case e_unit_type::expert_sugar_planter:
    case e_unit_type::expert_tobacco_planter:
    case e_unit_type::expert_cotton_planter:
    case e_unit_type::expert_fur_trapper:
    case e_unit_type::expert_lumberjack:
    case e_unit_type::expert_ore_miner:
    case e_unit_type::expert_silver_miner:
    case e_unit_type::master_carpenter:
    case e_unit_type::master_distiller:
    case e_unit_type::master_tobacconist:
    case e_unit_type::master_weaver:
    case e_unit_type::master_fur_trader:
    case e_unit_type::master_blacksmith:
    case e_unit_type::master_gunsmith:
    case e_unit_type::elder_statesman:
    case e_unit_type::firebrand_preacher:
    case e_unit_type::hardy_colonist:
    case e_unit_type::jesuit_colonist:
    case e_unit_type::seasoned_colonist:
    case e_unit_type::veteran_colonist:
    case e_unit_type::pioneer:
    case e_unit_type::hardy_pioneer:
      return e_dwelling_interaction_category::colonist;

    case e_unit_type::wagon_train:
    case e_unit_type::caravel:
    case e_unit_type::merchantman:
    case e_unit_type::galleon:
    case e_unit_type::privateer:
    case e_unit_type::frigate:
    case e_unit_type::man_o_war:
      return e_dwelling_interaction_category::trade;

    case e_unit_type::treasure:
      return e_dwelling_interaction_category::none;
  }
}

vector<e_enter_dwelling_option> const& options_for_unit(
    Player const& player, Tribe const& tribe,
    Dwelling const& dwelling, e_unit_type unit_type ) {
  static vector<e_enter_dwelling_option> const options_cancel{
      e_enter_dwelling_option::cancel,
  };
  if( !tribe.relationship[player.nation].has_value() ) {
    // The tribe has not yet made contact with us. This should
    // really only happen if we place a unit next to the dwelling
    // artificially.
    return options_cancel;
  }
  e_dwelling_interaction_category const category =
      dwelling_interaction_category( unit_type );
  UNWRAP_CHECK( relationship,
                tribe.relationship[player.nation] );
  bool const at_war = relationship.at_war;
  auto       switch_war =
      [&]( auto const& options,
           auto const& options_war ) -> decltype( auto ) {
    return at_war ? options_war : options;
  };
  switch( category ) {
    case e_dwelling_interaction_category::scout: {
      static vector<e_enter_dwelling_option> const options{
          e_enter_dwelling_option::speak_with_chief,
          e_enter_dwelling_option::attack_village,
          e_enter_dwelling_option::demand_tribute,
          e_enter_dwelling_option::cancel,
      };
      static vector<e_enter_dwelling_option> const
          options_at_war{
              e_enter_dwelling_option::speak_with_chief,
              e_enter_dwelling_option::attack_village,
              e_enter_dwelling_option::cancel,
          };
      return switch_war( options, options_at_war );
    }
    case e_dwelling_interaction_category::military: {
      static vector<e_enter_dwelling_option> const options{
          e_enter_dwelling_option::attack_village,
          e_enter_dwelling_option::demand_tribute,
          e_enter_dwelling_option::cancel,
      };
      static vector<e_enter_dwelling_option> const
          options_at_war{
              e_enter_dwelling_option::attack_village,
              e_enter_dwelling_option::cancel,
          };
      return switch_war( options, options_at_war );
    }
    case e_dwelling_interaction_category::colonist: {
      static vector<e_enter_dwelling_option> const options{
          e_enter_dwelling_option::live_among_the_natives,
          e_enter_dwelling_option::cancel,
      };
      return switch_war( options, options_cancel );
    }
    case e_dwelling_interaction_category::missionary: {
      static vector<e_enter_dwelling_option> const
          options_no_mission{
              e_enter_dwelling_option::establish_mission,
              e_enter_dwelling_option::incite_indians,
              e_enter_dwelling_option::cancel,
          };
      static vector<e_enter_dwelling_option> const
          options_has_mission{
              e_enter_dwelling_option::incite_indians,
              e_enter_dwelling_option::cancel,
          };
      auto const& options = ( dwelling.mission.has_value() )
                                ? options_has_mission
                                : options_no_mission;
      return switch_war( options, options_cancel );
    }
    case e_dwelling_interaction_category::trade: {
      static vector<e_enter_dwelling_option> const options{
          e_enter_dwelling_option::trade,
          e_enter_dwelling_option::cancel,
      };
      // In the OG, when attempting to trade with a tribe that is
      // hostile, it won't present any options to the player, it
      // will just state that the tribe does not wish to trade.
      // But that is implemented elsewhere.
      return switch_war( options, options );
    }
    case e_dwelling_interaction_category::none:
      return options_cancel;
  }
}

string const& reaction_str(
    e_enter_dwelling_reaction reaction ) {
  switch( reaction ) {
    case e_enter_dwelling_reaction::wave_happily: {
      static string const msg =
          "They wave to you @[H]happily@[].";
      return msg;
    }
    case e_enter_dwelling_reaction::wave_happily_with_scalps: {
      static string const msg =
          "They wave to you @[H]happily@[], though you notice "
          "some scalps hanging from trees in the distance.";
      return msg;
    }
    case e_enter_dwelling_reaction::frowning_archers: {
      static string const msg =
          "You notice small groups of warriors practicing "
          "archery and frowing at you with suspicion.";
      return msg;
    }
    case e_enter_dwelling_reaction::wary_warriors: {
      static string const msg =
          "You notice that there is no one in sight but for a "
          "band of warriors looking at your with wariness.";
      return msg;
    }
    case e_enter_dwelling_reaction::scalps_and_war_drums: {
      static string const msg =
          "You hear the sound of war drums as scalps hang from "
          "the trees.";
      return msg;
    }
  }
}

} // namespace

/****************************************************************
** General.
*****************************************************************/
EnterNativeDwellingOptions enter_native_dwelling_options(
    SSConst const& ss, Player const& player,
    e_unit_type unit_type, Dwelling const& dwelling ) {
  Tribe const& tribe = ss.natives.tribe_for( dwelling.tribe );
  return EnterNativeDwellingOptions{
      .dwelling_id = dwelling.id,
      .reaction =
          reaction_for_dwelling( ss, player, tribe, dwelling ),
      .category = dwelling_interaction_category( unit_type ),
      .options  = options_for_unit( player, tribe, dwelling,
                                    unit_type ) };
}

wait<base::NoDiscard<e_enter_dwelling_option>>
present_dwelling_entry_options(
    SSConst const& ss, TS& ts,
    EnterNativeDwellingOptions const& options ) {
  Dwelling const& dwelling =
      ss.natives.dwelling_for( options.dwelling_id );
  e_tribe const tribe = dwelling.tribe;
  string        msg   = fmt::format(
      "You have arrived at a {} of the @[H]{}@[].  {}",
      config_natives.tribes[tribe].dwelling_type,
      base::capitalize_initials( base::to_str( tribe ) ),
      reaction_str( options.reaction ) );
  maybe<e_enter_dwelling_option> const res =
      co_await ts.gui.partial_optional_enum_choice(
          msg, options.options );
  co_return res.value_or( e_enter_dwelling_option::cancel );
}

/****************************************************************
** Live Among the Natives.
*****************************************************************/
LiveAmongTheNatives_t compute_live_among_the_natives(
    SSConst const& ss, TribeRelationship const& relationship,
    Dwelling const& dwelling, Unit const& unit ) {
  e_unit_type const base_type = unit.base_type();
  auto const&       attr      = unit_attr( base_type );
  if( !is_unit_human( unit.type_obj() ) )
    return LiveAmongTheNatives::generally_ineligible{};
  if( attr.expertise.has_value() )
    return LiveAmongTheNatives::has_expertise{
        .in_what = *attr.expertise };
  if( is_military_unit( unit.type() ) )
    // This should catch scouts and other military units.
    return LiveAmongTheNatives::generally_ineligible{};
  if( base_type == e_unit_type::petty_criminal )
    return LiveAmongTheNatives::petty_criminal{};
  if( base_type == e_unit_type::native_convert )
    return LiveAmongTheNatives::native_convert{};
  if( !dwelling.is_capital && dwelling.has_taught )
    return LiveAmongTheNatives::already_taught{};
  int const alarm =
      effective_dwelling_alarm( ss, dwelling, unit.nation() );
  if( alarm >
          config_natives.alarm.max_dwelling_alarm_for_teaching ||
      relationship.at_war )
    return LiveAmongTheNatives::unhappy{};
  UNWRAP_CHECK( new_comp,
                promoted_by_natives( unit.composition(),
                                     dwelling.teaches ) );
  CHECK( new_comp != unit.composition(), "{} == {}", new_comp,
         unit.composition() );
  return LiveAmongTheNatives::promoted{ .to = new_comp };
}

wait<> do_live_among_the_natives(
    TS& ts, Dwelling& dwelling, Player const& player, Unit& unit,
    LiveAmongTheNatives_t const& outcome ) {
  switch( outcome.to_enum() ) {
    using namespace LiveAmongTheNatives;
    case e::generally_ineligible: {
      co_await ts.gui.message_box(
          "This unit is not eligible for education by the "
          "Indians." );
      co_return;
    }
    case e::petty_criminal: {
      co_await ts.gui.message_box(
          "The unrefined ways of this @[H]Petty Criminal@[] "
          "resist any efforts of education.  Please bring us a "
          "@[H]Free Colonist@[] or an @[H]Indentured Servant@[] "
          "and we will teach them." );
      co_return;
    }
    case e::native_convert: {
      co_await ts.gui.message_box(
          "Native converts already know the ways of the "
          "Indians." );
      co_return;
    }
    case e::unhappy: {
      co_await ts.gui.message_box(
          "Your presence is offensive to us and not welcome "
          "here.  We will not teach you." );
      co_return;
    }
    case e::already_taught: {
      co_await ts.gui.message_box(
          "Friend, we have already shared our skills with the "
          "Europeans.  Other villages may yet have skills to "
          "share." );
      co_return;
    }
    case e::has_expertise: {
      co_await ts.gui.message_box(
          "Friend, you are a great @[H]{}@[].  We can only "
          "teach new skills to unskilled colonists.  Bring us a "
          "@[H]Free Colonist@[] or @[H]Indentured Servant@[] "
          "and we will train them.",
          unit.desc().name );
      co_return;
    }
    case e::promoted: {
      auto&        o = outcome.get<promoted>();
      string const tribe_name =
          config_natives.tribes[dwelling.tribe].name_singular;
      string const msg = fmt::format(
          "Young colonist, we see that you have much to learn.  "
          "You may live among us and learn the ways of the "
          "@[H]{}@[] from the wise {} people.  Would you like "
          "to learn this skill?",
          unit_attr( o.to.base_type() ).name, tribe_name );
      maybe<ui::e_confirm> const yes_no =
          co_await ts.gui.optional_yes_no(
              YesNoConfig{ .msg            = msg,
                           .yes_label      = "Yes",
                           .no_label       = "No",
                           .no_comes_first = false } );
      if( yes_no == ui::e_confirm::yes ) {
        unit.change_type( player, o.to );
        dwelling.has_taught = true;
        co_await ts.gui.message_box(
            "Congratulations young one, you have learned the "
            "ways of the @[H]{}@[].",
            unit_attr( o.to.base_type() ).name );
        co_return;
      }
    }
  }
}

} // namespace rn
