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
#include "commodity.hpp"
#include "igui.hpp"
#include "imap-updater.hpp"
#include "irand.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "plane-stack.hpp"
#include "promotion.hpp"
#include "rand-enum.hpp"
#include "ts.hpp"
#include "visibility.hpp"
#include "woodcut.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/natives.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/dwelling.rds.hpp"
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/tribe.rds.hpp"
#include "ss/unit-type.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/iter.hpp"

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
    SSConst const& ss, Player const& player, Tribe const& tribe,
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
          // e_enter_dwelling_option::demand_tribute,
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
          // e_enter_dwelling_option::demand_tribute,
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
              // e_enter_dwelling_option::establish_mission,
              // e_enter_dwelling_option::incite_indians,
              e_enter_dwelling_option::cancel,
          };
      static vector<e_enter_dwelling_option> const
          options_has_mission{
              // e_enter_dwelling_option::incite_indians,
              e_enter_dwelling_option::cancel,
          };
      auto const& options =
          ( ss.units.missionary_from_dwelling( dwelling.id )
                .has_value() )
              ? options_has_mission
              : options_no_mission;
      return switch_war( options, options_cancel );
    }
    case e_dwelling_interaction_category::trade: {
      static vector<e_enter_dwelling_option> const options{
          // e_enter_dwelling_option::trade,
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
          "band of warriors looking at you with wariness.";
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

maybe<e_enter_dwelling_option>
adjust_village_attack_for_free_braves(
    SSConst const& ss, Dwelling const& dwelling,
    maybe<e_enter_dwelling_option> option ) {
  if( !option.has_value() ) return nothing;
  if( *option != e_enter_dwelling_option::attack_village )
    return option;
  CHECK( *option !=
         e_enter_dwelling_option::attack_brave_on_dwelling );
  unordered_set<GenericUnitId> const& braves_on_dwelling =
      ss.units.from_coord( ss.natives.coord_for( dwelling.id ) );
  if( !braves_on_dwelling.empty() )
    return e_enter_dwelling_option::attack_brave_on_dwelling;
  return option;
}

} // namespace

/****************************************************************
** General.
*****************************************************************/
EnterNativeDwellingOptions enter_native_dwelling_options(
    SSConst const& ss, Player const& player,
    e_unit_type unit_type, Dwelling const& dwelling ) {
  Tribe const& tribe = ss.natives.tribe_for( dwelling.id );
  return EnterNativeDwellingOptions{
      .dwelling_id = dwelling.id,
      .reaction =
          reaction_for_dwelling( ss, player, tribe, dwelling ),
      .options = options_for_unit( ss, player, tribe, dwelling,
                                   unit_type ) };
}

wait<e_enter_dwelling_option> present_dwelling_entry_options(
    SSConst const& ss, TS& ts, Player& player,
    EnterNativeDwellingOptions const& options ) {
  co_await display_woodcut_if_needed(
      ts, player, e_woodcut::entering_native_village );
  Dwelling const& dwelling =
      ss.natives.dwelling_for( options.dwelling_id );
  e_tribe const tribe = ss.natives.tribe_for( dwelling.id ).type;
  string        msg   = fmt::format(
      "You have arrived at a {} of the @[H]{}@[].  {}",
      config_natives
          .dwelling_types[config_natives.tribes[tribe].level]
          .name_singular,
      base::capitalize_initials( base::to_str( tribe ) ),
      reaction_str( options.reaction ) );
  maybe<e_enter_dwelling_option> res =
      co_await ts.gui.partial_optional_enum_choice(
          msg, options.options );
  res =
      adjust_village_attack_for_free_braves( ss, dwelling, res );
  co_return res.value_or( e_enter_dwelling_option::cancel );
}

/****************************************************************
** Live Among the Natives.
*****************************************************************/
LiveAmongTheNatives_t compute_live_among_the_natives(
    SSConst const& ss, Dwelling const& dwelling,
    Unit const& unit ) {
  UNWRAP_CHECK( relationship, ss.natives.tribe_for( dwelling.id )
                                  .relationship[unit.nation()] );
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
    Planes& planes, SSConst const& ss, TS& ts,
    Dwelling& dwelling, Player const& player, Unit& unit,
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
          unit_attr( unit.base_type() ).name );
      co_return;
    }
    case e::promoted: {
      auto&         o = outcome.get<promoted>();
      e_tribe const tribe =
          ss.natives.tribe_for( dwelling.id ).type;
      string const tribe_name =
          config_natives.tribes[tribe].name_singular;
      // This will allow us to show e.g. "Seasoned Scout" instead
      // of "Seasoned Colonist".
      string const new_name =
          unit_attr( unit_attr( o.to.base_type() )
                         .canonical_modified.value_or(
                             o.to.base_type() ) )
              .name;
      string const msg = fmt::format(
          "Young colonist, we see that you have much to learn.  "
          "You may live among us and learn the ways of the "
          "@[H]{}@[] from the wise {} people.  Would you like "
          "to learn this skill?",
          new_name, tribe_name );
      maybe<ui::e_confirm> const yes_no =
          co_await ts.gui.optional_yes_no(
              YesNoConfig{ .msg            = msg,
                           .yes_label      = "Yes",
                           .no_label       = "No",
                           .no_comes_first = false } );
      if( yes_no == ui::e_confirm::yes ) {
        if( o.to.type() != unit.type() )
          // We only animate the transition if there is one visi-
          // ble. Usually there is (e.g. free colonist changing
          // to expert farmer), but sometimes there is not, e.g.
          // when a pioneer's base type gets promoted but the
          // unit type (and hence unit sprite) remains as a pio-
          // neer.
          co_await planes.land_view().animate_unit_depixelation(
              PixelationAnimation::euro_unit_depixelate{
                  .id = unit.id(), .target = o.to.type() } );
        unit.change_type( player, o.to );
        dwelling.has_taught = true;
        co_await ts.gui.message_box(
            "Congratulations young one, you have learned the "
            "ways of the @[H]{}@[].",
            new_name );
        co_return;
      }
    }
  }
}

/****************************************************************
** Speak with Chief
*****************************************************************/
static vector<Coord> compute_tales_of_nearby_lands_tiles(
    SSConst const& ss, Dwelling const& dwelling,
    Unit const& unit, int radius ) {
  vector<Coord> tiles;
  tiles.reserve( radius * radius );
  // The standard values of the radius in the config are odd, so
  // the following should lead to a square centered on the
  // dwelling radius.
  Coord const location = ss.natives.coord_for( dwelling.id );
  Rect const  rect =
      Rect::from( location, Delta{ .w = 1, .h = 1 } )
          .with_border_added( radius / 2 );
  Visibility const viz = Visibility::create( ss, unit.nation() );
  for( auto r : gfx::subrects( rect ) ) {
    Coord const             coord = r.upper_left();
    maybe<MapSquare const&> square =
        ss.terrain.maybe_square_at( coord );
    // Square must exist on the map.
    if( !square.has_value() ) continue;
    // Square must not be visible to player.
    if( viz.visible( coord ) ) continue;
    // Square must be land.  This seems to be what the OG does.
    if( square->surface == e_surface::water ) continue;
    tiles.push_back( coord );
  }
  return tiles;
}

static ChiefAction_t compute_speak_with_chief_action(
    SSConst const& ss, TS& ts, Dwelling const& dwelling,
    Unit const& unit ) {
  UNWRAP_CHECK_MSG(
      type, scout_type( unit.type() ),
      "expected a scout or a seasoned scout but found type {}.",
      unit.type() );
  auto& conf = config_natives.speak_with_chief[type];

  // First determine if we should eliminate the scout ("target
  // practice"); note that this can happen even if the scout has
  // already spoken to the chief.
  config::IntRange const& elimination_range =
      conf.alarm_range_for_target_practice;
  int const alarm =
      effective_dwelling_alarm( ss, dwelling, unit.nation() );
  // Should have been validated during config loading.
  CHECK_GE( elimination_range.max, elimination_range.min );
  // Note that if max and min are equal, which they can be, then
  // we should never enter the third brance where we divide by
  // their difference (which would be zero).
  double const eliminate_probability =
      ( alarm >= elimination_range.max ) ? 1.0
      : ( alarm < elimination_range.min )
          ? 0.0
          : double( alarm - elimination_range.min ) /
                ( elimination_range.max -
                  elimination_range.min );
  lg.debug( "scout elimination probability: {}",
            eliminate_probability );
  bool const should_eliminate =
      ts.rand.bernoulli( eliminate_probability );
  if( should_eliminate ) return ChiefAction::target_practice{};

  // At this point we know that the scout will not be eliminated
  // and thus will get some kind of non-negative action from the
  // chief. However, that is only the first time that the scout
  // meets with the chief.
  if( dwelling.relationship[unit.nation()]
          .has_spoken_with_chief )
    return ChiefAction::none{};

  auto const outcome =
      pick_from_weighted_enum_values<e_speak_with_chief_result>(
          ts.rand, conf.positive_outcome_weights );
  switch( outcome ) {
    case e_speak_with_chief_result::none:
      return ChiefAction::none{};
    case e_speak_with_chief_result::gift_money: {
      e_tribe const tribe =
          ss.natives.tribe_for( dwelling.id ).type;
      config::IntRange const& gift_range =
          conf.gift_range[config_natives.tribes[tribe].level];
      int const quantity = ts.rand.between_ints(
          gift_range.min, gift_range.max, e_interval::closed );
      return ChiefAction::gift_money{ .quantity = quantity };
    }
    case e_speak_with_chief_result::tales_of_nearby_lands: {
      // If there are not enough non-visible tiles in the radius
      // then don't bother otherwise it might be confusing to the
      // player.
      int const     tales_size = conf.tales_of_nearby_land_size;
      vector<Coord> tiles = compute_tales_of_nearby_lands_tiles(
          ss, dwelling, unit, tales_size );
      if( ssize( tiles ) <= conf.min_invisible_tiles_for_tales )
        return ChiefAction::none{};
      return ChiefAction::tales_of_nearby_lands{
          .tiles = std::move( tiles ) };
    }
    case e_speak_with_chief_result::promotion: {
      // If the unit can't be promoted to seasoned scout then
      // just do nothing. This could happen if e.g. this is a
      // regular scout made from a unit that is an expert in
      // something other than scouting.
      bool const can_be_promoted =
          promoted_by_natives( unit.composition(),
                               e_native_skill::scouting )
              .has_value();
      lg.debug( "scout can be promoted: {}", can_be_promoted );
      if( !can_be_promoted ) return ChiefAction::none{};
      return ChiefAction::promotion{};
    }
  }
}

SpeakWithChiefResult compute_speak_with_chief(
    SSConst const& ss, TS& ts, Dwelling const& dwelling,
    Unit const& unit ) {
  return SpeakWithChiefResult{
      .expertise         = dwelling.teaches,
      .primary_trade     = dwelling.trading.seeking_primary,
      .secondary_trade_1 = dwelling.trading.seeking_secondary_1,
      .secondary_trade_2 = dwelling.trading.seeking_secondary_2,
      .action            = compute_speak_with_chief_action(
          ss, ts, dwelling, unit ) };
}

wait<> do_speak_with_chief(
    Planes& planes, SS& ss, TS& ts, Dwelling& dwelling,
    Player& player, Unit& unit,
    SpeakWithChiefResult const& outcome ) {
  dwelling.relationship[unit.nation()].has_spoken_with_chief =
      true;
  e_tribe const tribe = ss.natives.tribe_for( dwelling.id ).type;
  if( !outcome.action.holds<ChiefAction::target_practice>() )
    co_await ts.gui.message_box(
        "Greetings traveler, we are a peaceful @[H]{}@[] known "
        "for our expertise in @[H]{}@[]. We will trade with you "
        "if you bring us some @[H]{}@[], of which we are badly "
        "in need.  We would also accept @[H]{}@[] and "
        "@[H]{}@[].",
        config_natives
            .dwelling_types[config_natives.tribes[tribe].level]
            .name_singular,
        config_natives.native_skills[outcome.expertise]
            .display_name,
        lowercase_commodity_display_name(
            outcome.primary_trade ),
        lowercase_commodity_display_name(
            outcome.secondary_trade_1 ),
        lowercase_commodity_display_name(
            outcome.secondary_trade_2 ) );
  switch( outcome.action.to_enum() ) {
    case ChiefAction::e::none: {
      co_await ts.gui.message_box(
          "We always welcome @[H]{}@[] travelors.",
          config_nation.nations[unit.nation()].display_name );
      co_return;
    }
    case ChiefAction::e::gift_money: {
      auto const& o =
          outcome.action.get<ChiefAction::gift_money>();
      co_await ts.gui.message_box(
          "Please take these valuable beads (worth @[H]{}@[]) "
          "back to your chieftan.",
          o.quantity );
      player.money += o.quantity;
      co_return;
    }
    case ChiefAction::e::tales_of_nearby_lands: {
      auto const& o =
          outcome.action
              .get<ChiefAction::tales_of_nearby_lands>();
      CHECK( !o.tiles.empty() );
      auto tiles = o.tiles;
      ts.rand.shuffle( tiles );
      co_await ts.gui.message_box(
          "We invite you to sit around the campfire with us as "
          "we tell you the tales of nearby lands." );
      co_await planes.land_view().center_on_tile(
          ss.natives.coord_for( dwelling.id ) );
      for( Coord tile : tiles ) {
        ts.map_updater.make_square_visible( tile,
                                            unit.nation() );
        co_await ts.gui.wait_for( 20ms );
      }
      co_await ts.gui.wait_for( 600ms );
      break;
    }
    case ChiefAction::e::promotion: {
      co_await ts.gui.message_box(
          "To help our traveler friends we will send guides "
          "along with your scout." );
      co_await planes.land_view().animate_unit_depixelation(
          PixelationAnimation::euro_unit_depixelate{
              .id     = unit.id(),
              .target = e_unit_type::seasoned_scout } );
      // Need to change type before awaiting on the promotion
      // message otherwise the unit will change back temporarily
      // after depixelating.
      unit.change_type( player,
                        UnitComposition::create(
                            e_unit_type::seasoned_scout ) );
      co_await ts.gui.message_box(
          "Our scout has been promoted to @[H]Seasoned "
          "Scout@[]!" );
      co_return;
    }
    case ChiefAction::e::target_practice: {
      co_await ts.gui.message_box(
          "You have violated sacred taboos of the @[H]{}@[] "
          "tribe and thus we have decided to use your scout as "
          "target practice.",
          config_natives.tribes[tribe].name_singular );
      co_await planes.land_view().animate_unit_depixelation(
          PixelationAnimation::euro_unit_depixelate{
              .id = unit.id(), .target = nothing } );
      ss.units.destroy_unit( unit.id() );
      co_return;
    }
  }
}

AttackVillageResult compute_attack_village(
    SSConst const& ss, TS& ts, Player const& player,
    Dwelling const& dwelling, Unit const& attacker ) {
  CHECK_EQ( attacker.nation(), player.nation );
  AttackVillageResult res;

  return res;
}

wait<> do_attack_village( Planes& planes, SS& ss, TS& ts,
                          Dwelling& dwelling, Player& player,
                          Unit&                      unit,
                          AttackVillageResult const& outcome ) {
  co_return;
}

} // namespace rn
