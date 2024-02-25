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
#include "anim-builders.hpp"
#include "co-wait.hpp"
#include "harbor-units.hpp"
#include "imap-updater.hpp"
#include "immigration.hpp"
#include "irand.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "plane-stack.hpp"
#include "rand-enum.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unit-ownership.hpp"

// ss
#include "ss/old-world-state.rds.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/unit-type.hpp"
#include "ss/units.hpp"

// config
#include "config/lcr.rds.hpp"
#include "config/text.rds.hpp"

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

// When exploring burial mounds that are in native owned land we
// could find indian burial grounds, which will cause that tribe
// to declare war (permanently?).
bool is_native_land() {
  // TODO
  return false;
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

wait<LostCityRumorResult> run_burial_mounds_result(
    e_burial_mounds_type type, bool has_burial_grounds, SS& ss,
    TS& ts, Player& player, UnitId unit_id,
    Coord world_square ) {
  LostCityRumorResult           result = {};
  e_lcr_explorer_category const explorer =
      lcr_explorer_category( ss.units, unit_id );
  switch( type ) {
    case e_burial_mounds_type::trinkets: {
      int amount = random_gift(
          ts.rand,
          { .min      = config_lcr.trinkets_gift_min[explorer],
            .max      = config_lcr.trinkets_gift_max[explorer],
            .multiple = config_lcr.trinkets_gift_multiple } );
      co_await ts.gui.message_box(
          "You've found some trinkets worth [{}{}].", amount,
          config_text.special_chars.currency );
      int total = player.money += amount;
      lg.info(
          "{} gold added to {} treasury.  current balance: {}.",
          amount, player.nation, total );
      result = LostCityRumorResult::other{};
      break;
    }
    case e_burial_mounds_type::treasure_train: {
      int amount = random_gift(
          ts.rand,
          { .min =
                config_lcr.burial_mounds_treasure_min[explorer],
            .max =
                config_lcr.burial_mounds_treasure_max[explorer],
            .multiple =
                config_lcr.burial_mounds_treasure_multiple } );
      co_await ts.gui.message_box(
          "You've recovered a treasure worth [{}{}]!", amount,
          config_text.special_chars.currency );
      UnitId const unit_id = create_treasure_train(
          ss, ts, player, world_square, amount );
      co_await ts.planes.land_view().animate(
          anim_seq_for_treasure_enpixelation( ss, unit_id ) );
      result =
          LostCityRumorResult::unit_created{ .id = unit_id };
      break;
    }
    case e_burial_mounds_type::cold_and_empty: {
      co_await ts.gui.message_box(
          "The mounds are cold and empty." );
      result = LostCityRumorResult::other{};
      break;
    }
  }
  if( has_burial_grounds ) {
    co_await ts.gui.message_box(
        "These are native burial grounds.  WAR!" );
  }
  co_return result;
}

wait<> take_one_immigrant( SS& ss, TS& ts, Player& player,
                           SettingsState const& settings ) {
  Player const& cplayer = player;
  // NOTE: The original game seems to always allow the player to
  // choose from the three immigrants in the pool for each round
  // when they come via the fountain of youth. This is in con-
  // trast to immigration via crosses which only allows the
  // player to choose when William Brewster has been obtained.
  maybe<int> choice = co_await ask_player_to_choose_immigrant(
      ts.gui, cplayer.old_world.immigration,
      "Who shall we next choose to join us in the New "
      "World?" );
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
    co_await take_one_immigrant( ss, ts, player, settings );
    // If we don't do this then the next window pops up instanta-
    // neously and its visually confusing since it's not clear
    // that the first window closed and a new one popped up, it
    // gives the impression that the first selection did not
    // work. Do this with IGui so that we don't make life diffi-
    // cult for unit tests.
    co_await ts.gui.wait_for( chrono::milliseconds( 300 ) );
  }
}

wait<LostCityRumorResult> run_rumor_result(
    e_rumor_type type, e_burial_mounds_type burial_type,
    bool has_burial_grounds, SS& ss, TS& ts, Player& player,
    UnitId unit_id, Coord world_square ) {
  e_lcr_explorer_category const explorer =
      lcr_explorer_category( ss.units, unit_id );
  switch( type ) {
    case e_rumor_type::none: {
      co_await ts.gui.message_box(
          "You find nothing but rumors." );
      co_return LostCityRumorResult::other{};
    }
    case e_rumor_type::fountain_of_youth: {
      co_await run_fountain_of_youth( ss, ts, player,
                                      ss.settings );
      co_return LostCityRumorResult::other{};
    }
    case e_rumor_type::ruins: {
      int amount = random_gift(
          ts.rand,
          { .min      = config_lcr.ruins_gift_min[explorer],
            .max      = config_lcr.ruins_gift_max[explorer],
            .multiple = config_lcr.ruins_gift_multiple } );
      co_await ts.gui.message_box(
          "You've discovered the ruins of a lost colony, among "
          "which there are items worth [{}{}].",
          amount, config_text.special_chars.currency );
      player.money += amount;
      int total = player.money;
      lg.info(
          "{} gold added to {} treasury.  current balance: {}.",
          amount, player.nation, total );
      co_return LostCityRumorResult::other{};
    }
    case e_rumor_type::burial_mounds: {
      ui::e_confirm res = co_await ts.gui.required_yes_no(
          { .msg = "You stumble across some mysterious ancient "
                   "burial mounds.  Explore them?",
            .yes_label      = "Let us search for treasure!",
            .no_label       = "Leave them alone.",
            .no_comes_first = false } );
      if( res == ui::e_confirm::no )
        co_return LostCityRumorResult::other{};
      LostCityRumorResult result =
          co_await run_burial_mounds_result(
              burial_type, has_burial_grounds, ss, ts, player,
              unit_id, world_square );
      co_return result;
    }
    case e_rumor_type::chief_gift: {
      int amount = random_gift(
          ts.rand,
          { .min      = config_lcr.chief_gift_min[explorer],
            .max      = config_lcr.chief_gift_max[explorer],
            .multiple = config_lcr.chief_gift_multiple } );
      co_await ts.gui.message_box(
          "You happen upon a small village.  The chief offers "
          "you a gift worth [{}{}].",
          amount, config_text.special_chars.currency );
      player.money += amount;
      int total = player.money;
      lg.info(
          "{} gold added to {} treasury.  current balance: {}.",
          amount, player.nation, total );
      co_return LostCityRumorResult::other{};
    }
    case e_rumor_type::free_colonist: {
      co_await ts.gui.message_box(
          "You happen upon the survivors of a lost colony.  In "
          "exchange for badly-needed supplies, they agree to "
          "swear allegiance to you and join your expedition." );
      // Use the non-coroutine version of this because it avoids
      // an inifinite-regress issue where the new unit created
      // rediscovers the LCR on this tile; also, there are no
      // further UI actions needed in response to creating this
      // unit, apart from what we will do here.
      UnitId id = create_unit_on_map_non_interactive(
          ss, ts, player, e_unit_type::free_colonist,
          world_square );
      co_return LostCityRumorResult::unit_created{ .id = id };
    }
    case e_rumor_type::unit_lost: {
      // Destroy unit before showing message so that the unit ac-
      // tually appears to disappear.
      UnitOwnershipChanger( ss, unit_id ).destroy();
      co_await ts.gui.message_box(
          "Our colonist has vanished without a trace." );
      co_return LostCityRumorResult::unit_lost{};
    }
    case e_rumor_type::cibola: {
      int amount = random_gift(
          ts.rand,
          { .min      = config_lcr.cibola_treasure_min[explorer],
            .max      = config_lcr.cibola_treasure_max[explorer],
            .multiple = config_lcr.cibola_treasure_multiple } );
      co_await ts.gui.message_box(
          "You've discovered one of the [Seven Cities of "
          "Cibola] and have recovered a treasure worth [{}{}]!",
          amount, config_text.special_chars.currency );
      UnitId unit_id = create_treasure_train(
          ss, ts, player, world_square, amount );
      co_await ts.planes.land_view().animate(
          anim_seq_for_treasure_enpixelation( ss, unit_id ) );
      co_return LostCityRumorResult::unit_created{ .id =
                                                       unit_id };
    }
  }
  SHOULD_NOT_BE_HERE;
}

} // namespace

bool has_lost_city_rumor( TerrainState const& terrain_state,
                          Coord               square ) {
  return terrain_state.square_at( square ).lost_city_rumor;
}

e_lcr_explorer_category lcr_explorer_category(
    UnitsState const& units_state, UnitId unit_id ) {
  Unit const& unit = units_state.unit_for( unit_id );
  switch( unit.type() ) {
    case e_unit_type::seasoned_scout:
      return e_lcr_explorer_category::seasoned_scout;
    case e_unit_type::scout:
      return e_lcr_explorer_category::scout;
    default:
      return e_lcr_explorer_category::other;
  }
}

e_burial_mounds_type pick_burial_mounds_result(
    IRand& rand, e_lcr_explorer_category explorer ) {
  refl::enum_map<e_burial_mounds_type, int> const& weights =
      config_lcr.burial_mounds_type_weights[explorer];
  return pick_from_weighted_enum_values( rand, weights );
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
    weights[e_rumor_type::unit_lost] = 0;
  }

  // Make sure, after having removed some of the possibilities,
  // that we still have some non-zero weights for at least one
  // outcome. If we don't, which could happen in a modded game,
  // then the below will crash. So in that case we just default
  // to "no outcome."
  int sum = 0;
  for( auto [_, weight] : weights ) sum += weight;
  if( sum == 0 ) weights[e_rumor_type::none] = 100;

  return pick_from_weighted_enum_values( rand, weights );
}

bool pick_burial_grounds_result(
    IRand& rand, Player const& player,
    e_lcr_explorer_category explorer,
    e_burial_mounds_type    burial_type ) {
  if( !is_native_land() ) return false;
  bool positive_burial_mounds_result =
      ( burial_type != e_burial_mounds_type::cold_and_empty );
  // TODO: Some say that suppressing burial grounds via De Soto
  // requires using a scout (need to determine this). Also, some
  // say that even with De Soto you can still stumble on native
  // burial grounds, though that would always be accompanied by a
  // positive result otherwise such as a treasure, hence the
  // logic below. In that way, De Soto only means that "purely
  // negative" results are prevented. But this needs to be deter-
  // mined.
  bool allow_negative = !has_hernando_de_soto( player ) ||
                        positive_burial_mounds_result;
  if( !allow_negative ) return false;
  // We are clear for allowing burial grounds. But whether we
  // trigger it is still a matter of probability.
  return rand.bernoulli(
      config_lcr.burial_grounds_probability[explorer]
          .probability );
}

wait<LostCityRumorResult> run_lost_city_rumor_result(
    SS& ss, TS& ts, Player& player, UnitId unit_id,
    Coord world_square, e_rumor_type type,
    e_burial_mounds_type burial_type, bool has_burial_grounds ) {
  LostCityRumorResult result = co_await run_rumor_result(
      type, burial_type, has_burial_grounds, ss, ts, player,
      unit_id, world_square );
  // Remove lost city rumor.
  ts.map_updater.modify_map_square(
      world_square, []( MapSquare& square ) {
        CHECK_EQ( square.lost_city_rumor, true );
        square.lost_city_rumor = false;
      } );
  co_return result;
}

} // namespace rn
