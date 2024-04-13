/****************************************************************
**lcr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-19.
*
* Description: All things related to Lost City Rumors.
*
*****************************************************************/
#include "lcr.hpp"

// Revolution Now
#include "alarm.hpp"
#include "anim-builders.hpp"
#include "co-wait.hpp"
#include "harbor-units.hpp"
#include "igui.hpp"
#include "imap-search.rds.hpp"
#include "imap-updater.hpp"
#include "immigration.hpp"
#include "irand.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "plane-stack.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unit-ownership.hpp"

// config
#include "config/lcr.rds.hpp"
#include "config/natives.rds.hpp"
#include "config/text.rds.hpp"

// ss
#include "ss/natives.hpp"
#include "ss/old-world-state.rds.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/unit-type.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/enum-map.hpp"
#include "refl/to-str.hpp"

// base
#include "base/no-default.hpp"

using namespace std;

namespace rn {

namespace {

struct GiftOptions {
  int min      = base::no_default<>;
  int max      = base::no_default<>;
  int multiple = base::no_default<>;
};

int random_gift( IRand& rand, GiftOptions options ) {
  if( options.multiple < 1 ) options.multiple = 1;
  if( options.min < options.multiple )
    options.min = options.multiple;
  if( options.max < options.min ) options.max = options.min;
  int amount = rand.between_ints( options.min, options.max );
  return ( amount / options.multiple ) * options.multiple;
}

bool has_hernando_de_soto( Player const& player ) {
  return player.fathers.has[e_founding_father::hernando_de_soto];
}

// The fountain of youth is only allowed pre-independence.
bool allow_fountain_of_youth( Player const& player ) {
  return player.revolution_status ==
         e_revolution_status::not_declared;
}

UnitId create_treasure_train( SS& ss, TS& ts,
                              Player const& player,
                              Coord world_square, int amount ) {
  UNWRAP_CHECK( uc_treasure,
                UnitComposition::create(
                    e_unit_type::treasure,
                    { { e_unit_inventory::gold, amount } } ) );
  // Use the non-coroutine version of this because it avoids an
  // inifinite-regress issue where the new unit created redis-
  // covers the LCR on this tile; also, there are no further UI
  // actions needed in response to creating this unit, apart from
  // what we will do here.
  return create_unit_on_map_non_interactive(
      ss, ts, player, uc_treasure, world_square );
}

wait<LostCityRumorUnitChange> run_burial_mounds_result(
    SS& ss, TS& ts, Player& player, Coord world_square,
    BurialMounds const& mounds, maybe<e_tribe> burial_grounds ) {
  LostCityRumorUnitChange result = {};
  SWITCH( mounds ) {
    CASE( cold_and_empty ) {
      co_await ts.gui.message_box(
          "The mounds are cold and empty." );
      result = LostCityRumorUnitChange::other{};
      break;
    }
    CASE( treasure ) {
      co_await ts.gui.message_box(
          "You've recovered a treasure worth [{}{}]!",
          treasure.gold, config_text.special_chars.currency );
      UnitId const unit_id = create_treasure_train(
          ss, ts, player, world_square, treasure.gold );
      co_await ts.planes.land_view().animate(
          anim_seq_for_treasure_enpixelation( ss, unit_id ) );
      result =
          LostCityRumorUnitChange::unit_created{ .id = unit_id };
      break;
    }
    CASE( trinkets ) {
      co_await ts.gui.message_box(
          "You've found some trinkets worth [{}{}].",
          trinkets.gold, config_text.special_chars.currency );
      int total = player.money += trinkets.gold;
      lg.info(
          "{} gold added to {} treasury.  current balance: {}.",
          trinkets.gold, player.nation, total );
      result = LostCityRumorUnitChange::other{};
      break;
    }
  }

  if( burial_grounds.has_value() ) {
    // This appears to bump the tribal alarm toward the player to
    // 100 on every difficulty level.
    e_tribe const tribe_type = *burial_grounds;
    Tribe&        tribe = ss.natives.tribe_for( tribe_type );
    co_await ts.gui.message_box(
        "You have treaded on the sacred resting places of the "
        "ancestors of the [{}] tribe. Your irreverence shall be "
        "put to an end... prepare for WAR!",
        config_natives.tribes[tribe_type].name_singular );
    // Do this after so that the exclamation marks on the
    // dwellings appear after the message box closes.
    increase_tribal_alarm_from_burial_ground_trespass(
        as_const( player ), tribe.relationship[player.nation] );
  }
  co_return result;
}

wait<> take_one_immigrant( SS& ss, TS& ts, Player& player,
                           SettingsState const& settings,
                           int total, int idx ) {
  Player const& cplayer = player;
  // NOTE: The original game seems to always allow the player to
  // choose from the three immigrants in the pool for each round
  // when they come via the fountain of youth. This is in con-
  // trast to immigration via crosses which only allows the
  // player to choose when William Brewster has been obtained.
  maybe<int> choice = co_await ask_player_to_choose_immigrant(
      ts.gui, cplayer.old_world.immigration,
      fmt::format( "Who shall we choose as immigrant number "
                   "[{}] out of {} to join us in the New World?",
                   idx + 1, total ) );
  // The original game allows escaping from each prompt and that
  // will skip to the next one without adding an immigrant.
  if( !choice.has_value() ) co_return;
  e_unit_type replacement =
      pick_next_unit_for_pool( ts.rand, cplayer, settings );
  e_unit_type taken = take_immigrant_from_pool(
      player.old_world.immigration, *choice, replacement );
  create_unit_in_harbor( ss, player, taken );
}

wait<> run_fountain_of_youth( SS& ss, TS& ts, Player& player,
                              SettingsState const& settings ) {
  co_await ts.gui.message_box(
      "You've discovered a Fountain of Youth!" );
  int const count = config_lcr.fountain_of_youth_num_immigrants;
  for( int i = 0; i < count; ++i ) {
    co_await take_one_immigrant( ss, ts, player, settings, count,
                                 i );
    // Give a little visual indicator that there are multiple
    // windows popping up.
    co_await ts.gui.wait_for( chrono::milliseconds( 100 ) );
  }
}

wait<LostCityRumorUnitChange> run_rumor_result(
    SS& ss, TS& ts, Player& player, Unit const& unit, Coord tile,
    LostCityRumor const& rumor ) {
  SWITCH( rumor ) {
    CASE( unit_lost ) {
      // Destroy unit before showing message so that the unit ac-
      // tually appears to disappear.
      UnitOwnershipChanger( ss, unit.id() ).destroy();
      co_await ts.gui.message_box(
          "Our colonist has vanished without a trace." );
      co_return LostCityRumorUnitChange::unit_lost{};
    }
    CASE( burial_mounds ) {
      ui::e_confirm res = co_await ts.gui.required_yes_no(
          { .msg = "You stumble across some mysterious ancient "
                   "burial mounds.  Explore them?",
            .yes_label      = "Let us search for treasure!",
            .no_label       = "Leave them alone.",
            .no_comes_first = false } );
      if( res == ui::e_confirm::no )
        co_return LostCityRumorUnitChange::other{};
      LostCityRumorUnitChange const result =
          co_await run_burial_mounds_result(
              ss, ts, player, tile, burial_mounds.mounds,
              burial_mounds.burial_grounds );
      co_return result;
    }
    CASE( chief_gift ) {
      co_await ts.gui.message_box(
          "You happen upon a small village.  The chief offers "
          "you a gift worth [{}{}].",
          chief_gift.gold, config_text.special_chars.currency );
      player.money += chief_gift.gold;
      co_return LostCityRumorUnitChange::other{};
    }
    CASE( cibola ) {
      co_await ts.gui.message_box(
          "You've discovered one of the [Seven Cities of "
          "Cibola] and have recovered a treasure worth [{}{}]!",
          cibola.gold, config_text.special_chars.currency );
      UnitId unit_id = create_treasure_train(
          ss, ts, player, tile, cibola.gold );
      co_await ts.planes.land_view().animate(
          anim_seq_for_treasure_enpixelation( ss, unit_id ) );
      co_return LostCityRumorUnitChange::unit_created{
          .id = unit_id };
    }
    CASE( fountain_of_youth ) {
      co_await run_fountain_of_youth( ss, ts, player,
                                      ss.settings );
      co_return LostCityRumorUnitChange::other{};
    }
    CASE( free_colonist ) {
      co_await ts.gui.message_box(
          "You happen upon the survivors of a lost colony.  In "
          "exchange for badly-needed supplies, they agree to "
          "swear allegiance to you and join your expedition." );
      // Use the non-coroutine version of this because it avoids
      // an inifinite-regress issue where the new unit created
      // rediscovers the LCR on this tile; also, there are no
      // further UI actions needed in response to creating this
      // unit, apart from what we will do here.
      UnitId const unit_id = create_unit_on_map_non_interactive(
          ss, ts, player, e_unit_type::free_colonist, tile );
      co_return LostCityRumorUnitChange::unit_created{
          .id = unit_id };
    }
    CASE( holy_shrines ) {
      e_tribe const tribe_type = holy_shrines.tribe;
      Tribe&        tribe = ss.natives.tribe_for( tribe_type );
      co_await ts.gui.message_box(
          "You are wondering dangerously close to the holy "
          "shrines of the [{}] tribe... the [{}] tribe has been "
          "angered.",
          config_natives.tribes[tribe_type].name_singular,
          config_natives.tribes[tribe_type].name_singular );
      // Do this after so that the exclamation marks on the
      // dwellings appear after the message box closes.
      increase_tribal_alarm(
          player, holy_shrines.alarm_increase,
          tribe.relationship[player.nation].tribal_alarm );
      co_return LostCityRumorUnitChange::other{};
    }
    CASE( none ) {
      co_await ts.gui.message_box(
          "You find nothing but rumors." );
      co_return LostCityRumorUnitChange::other{};
    }
    CASE( ruins ) {
      co_await ts.gui.message_box(
          "You've discovered the ruins of a lost colony, among "
          "which there are items worth [{}{}].",
          ruins.gold, config_text.special_chars.currency );
      player.money += ruins.gold;
      co_return LostCityRumorUnitChange::other{};
    }
  }

  SHOULD_NOT_BE_HERE;
}

e_lcr_explorer_category lcr_explorer_category(
    e_unit_type unit_type ) {
  switch( unit_type ) {
    case e_unit_type::seasoned_scout:
      return e_lcr_explorer_category::seasoned_scout;
    case e_unit_type::scout:
      return e_lcr_explorer_category::scout;
    default:
      return e_lcr_explorer_category::other;
  }
}

e_burial_mounds_type pick_burial_mounds_type(
    IRand& rand, e_lcr_explorer_category explorer ) {
  refl::enum_map<e_burial_mounds_type, int> const& weights =
      config_lcr.burial_mounds_type_weights[explorer];
  return rand.pick_from_weighted_values( weights );
}

e_rumor_type pick_rumor_type_result(
    IRand& rand, e_lcr_explorer_category explorer,
    Player const& player ) {
  refl::enum_map<e_rumor_type, int> weights =
      config_lcr.rumor_type_weights[explorer];

  if( !allow_fountain_of_youth( player ) )
    weights[e_rumor_type::fountain_of_youth] = 0;

  if( has_hernando_de_soto( player ) ) {
    // Most docs say that having De Soto will prevent LCR results
    // from being negative. This means that you can still get
    // neutral results, such as "nothing but rumors", but the
    // solely-negative results are prevented.
    //
    // TODO: However, some say that this requires using a scout
    // (need to determine this).
    weights[e_rumor_type::unit_lost]    = 0;
    weights[e_rumor_type::holy_shrines] = 0;
  }

  // Make sure, after having removed some of the possibilities,
  // that we still have some non-zero weights for at least one
  // outcome. If we don't, which could happen in a modded game,
  // then the below will crash. So in that case we just default
  // to "no outcome."
  int sum = 0;
  for( auto [_, weight] : weights ) sum += weight;
  if( sum == 0 ) weights[e_rumor_type::none] = 100;

  return rand.pick_from_weighted_values( weights );
}

maybe<e_tribe> pick_burial_grounds_result(
    IRand& rand, e_tribe close_encountered_tribe,
    Player const& player, e_lcr_explorer_category explorer,
    e_burial_mounds_type burial_mounds ) {
  bool const positive_burial_mounds_result =
      ( burial_mounds != e_burial_mounds_type::cold_and_empty );
  // TODO: Some say that suppressing burial grounds via De Soto
  // requires using a scout (need to determine this).
  //
  // Confirmed in the OG that without de soto we can get burial
  // grounds on any of the mounds results, and with de soto, we
  // can get burial grounds on the positive results but not the
  // negative results. In this way, De Soto only means that
  // "purely negative" results are prevented.
  bool const allow_negative = !has_hernando_de_soto( player ) ||
                              positive_burial_mounds_result;
  if( !allow_negative ) return nothing;
  // We are clear for allowing burial grounds. But whether we
  // trigger it is still a matter of probability.
  return rand.bernoulli(
             config_lcr.burial_grounds_probability[explorer]
                 .probability )
             ? close_encountered_tribe
             : maybe<e_tribe>();
}

BurialMounds compute_mounds( e_burial_mounds_type    type,
                             e_lcr_explorer_category explorer,
                             IRand&                  rand ) {
  switch( type ) {
    case e_burial_mounds_type::cold_and_empty:
      return BurialMounds::cold_and_empty{};
    case e_burial_mounds_type::treasure_train: {
      int const gold = random_gift(
          rand,
          { .min =
                config_lcr.burial_mounds_treasure_min[explorer],
            .max =
                config_lcr.burial_mounds_treasure_max[explorer],
            .multiple =
                config_lcr.burial_mounds_treasure_multiple } );
      return BurialMounds::treasure{ .gold = gold };
    }
    case e_burial_mounds_type::trinkets: {
      int const gold = random_gift(
          rand,
          { .min      = config_lcr.trinkets_gift_min[explorer],
            .max      = config_lcr.trinkets_gift_max[explorer],
            .multiple = config_lcr.trinkets_gift_multiple } );
      return BurialMounds::trinkets{ .gold = gold };
    }
  }
}

LostCityRumor compute_rumor_type(
    e_difficulty const            difficulty,
    e_lcr_explorer_category const explorer,
    e_rumor_type const rumor_type, Player const& player,
    Coord const tile, IRand& rand,
    IMapSearch const& map_search ) {
  switch( rumor_type ) {
    case e_rumor_type::unit_lost: {
      return LostCityRumor::unit_lost{};
    }
    case e_rumor_type::burial_mounds: {
      e_burial_mounds_type const mounds_type =
          pick_burial_mounds_type( rand, explorer );
      BurialMounds const mounds =
          compute_mounds( mounds_type, explorer, rand );
      auto const burial_grounds = [&]() -> maybe<e_tribe> {
        maybe<e_tribe> const close_encountered_tribe =
            map_search.find_close_encountered_tribe(
                player.nation, tile,
                config_lcr.burial_grounds_radius );
        if( close_encountered_tribe.has_value() )
          return pick_burial_grounds_result(
              rand, *close_encountered_tribe, player, explorer,
              mounds_type );
        return nothing;
      }();
      return LostCityRumor::burial_mounds{
          .mounds = mounds, .burial_grounds = burial_grounds };
    }
    case e_rumor_type::chief_gift: {
      int const gold = random_gift(
          rand, { .min = config_lcr.chief_gift_min[explorer],
                  .max = config_lcr.chief_gift_max[explorer],
                  .multiple = config_lcr.chief_gift_multiple } );
      return LostCityRumor::chief_gift{ .gold = gold };
    }
    case e_rumor_type::cibola: {
      int const gold = random_gift(
          rand,
          { .min      = config_lcr.cibola_treasure_min[explorer],
            .max      = config_lcr.cibola_treasure_max[explorer],
            .multiple = config_lcr.cibola_treasure_multiple } );
      return LostCityRumor::cibola{ .gold = gold };
    }
    case e_rumor_type::fountain_of_youth: {
      return LostCityRumor::fountain_of_youth{};
    }
    case e_rumor_type::free_colonist: {
      return LostCityRumor::free_colonist{};
    }
    case e_rumor_type::holy_shrines: {
      maybe<e_tribe> const close_encountered_tribe =
          map_search.find_close_encountered_tribe(
              player.nation, tile,
              config_lcr.burial_grounds_radius );
      if( close_encountered_tribe.has_value() ) {
        auto const& range =
            config_lcr.holy_shrines_alarm_increase[difficulty];
        int const alarm_increase =
            rand.between_ints( range.min, range.max );
        return LostCityRumor::holy_shrines{
            .tribe          = *close_encountered_tribe,
            .alarm_increase = alarm_increase };
      } else {
        return LostCityRumor::none{};
      }
    }
    case e_rumor_type::none: {
      return LostCityRumor::none{};
    }
    case e_rumor_type::ruins: {
      int const gold = random_gift(
          rand, { .min = config_lcr.ruins_gift_min[explorer],
                  .max = config_lcr.ruins_gift_max[explorer],
                  .multiple = config_lcr.ruins_gift_multiple } );
      return LostCityRumor::ruins{ .gold = gold };
    }
  }
}

} // namespace

LostCityRumor compute_lcr( SSConst const& ss,
                           Player const& player, IRand& rand,
                           IMapSearch const& map_search,
                           e_unit_type unit_type, Coord tile ) {
  e_lcr_explorer_category const explorer =
      lcr_explorer_category( unit_type );
  e_rumor_type const rumor_type =
      pick_rumor_type_result( rand, explorer, player );
  return compute_rumor_type( ss.settings.difficulty, explorer,
                             rumor_type, player, tile, rand,
                             map_search );
}

wait<LostCityRumorUnitChange> run_lcr(
    SS& ss, TS& ts, Player& player, Unit const& unit,
    Coord world_square, LostCityRumor const& rumor ) {
  LostCityRumorUnitChange result = co_await run_rumor_result(
      ss, ts, player, unit, world_square, rumor );
  // Remove lost city rumor.
  ts.map_updater.modify_map_square(
      world_square, []( MapSquare& square ) {
        CHECK_EQ( square.lost_city_rumor, true );
        square.lost_city_rumor = false;
      } );
  co_return result;
}

} // namespace rn
