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
#include "co-wait.hpp"
#include "gs-terrain.hpp"
#include "gs-units.hpp"
#include "harbor-units.hpp"
#include "logger.hpp"
#include "rand-enum.hpp"
#include "rand.hpp"
#include "ustate.hpp"
#include "utype.hpp"
#include "window.hpp"

// config
#include "config/lcr.rds.hpp"

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

int random_gift( GiftOptions options ) {
  if( options.multiple < 1 ) options.multiple = 1;
  if( options.min < options.multiple )
    options.min = options.multiple;
  if( options.max < options.min ) options.max = options.min;
  int amount = rng::between( options.min, options.max,
                             rng::e_interval::closed );
  return ( amount / options.multiple ) * options.multiple;
}

bool has_hernando_de_soto( Player const& player ) {
  return player.has_father(
      e_founding_father::hernando_de_soto );
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
  return !player.independence_declared();
}

wait<LostCityRumorResult_t> run_burial_mounds_result(
    e_burial_mounds_type type, bool has_burial_grounds,
    UnitsState& units_state, IGui& gui, Player& player,
    IMapUpdater& map_updater, UnitId unit_id,
    Coord world_square ) {
  LostCityRumorResult_t         result = {};
  e_lcr_explorer_category const explorer =
      lcr_explorer_category( units_state, unit_id );
  switch( type ) {
    case e_burial_mounds_type::trinkets: {
      int amount = random_gift(
          { .min      = config_lcr.trinkets_gift_min[explorer],
            .max      = config_lcr.trinkets_gift_max[explorer],
            .multiple = config_lcr.trinkets_gift_multiple } );
      co_await gui.message_box(
          "You've found some trinkets worth @[H]{}@[] gold.",
          amount );
      int total = player.add_money( amount );
      lg.info(
          "{} gold added to {} treasury.  current balance: {}.",
          amount, player.nation(), total );
      result = LostCityRumorResult::other{};
      break;
    }
    case e_burial_mounds_type::treasure_train: {
      int amount = random_gift(
          { .min =
                config_lcr.burial_mounds_treasure_min[explorer],
            .max =
                config_lcr.burial_mounds_treasure_max[explorer],
            .multiple =
                config_lcr.burial_mounds_treasure_multiple } );
      co_await gui.message_box(
          "You've recovered a treasure worth @[H]{}@[].",
          amount );
      UNWRAP_CHECK(
          uc_treasure,
          UnitComposition::create(
              UnitType::create( e_unit_type::large_treasure ),
              { { e_unit_inventory::gold, amount } } ) );
      // Use the non-coroutine version of this because it avoids
      // an inifinite-regress issue where the new unit created
      // rediscovers the LCR on this tile; also, there are no
      // further UI actions needed in response to creating this
      // unit, apart from what we will do here.
      UnitId id = create_unit_on_map_no_ui(
          units_state, map_updater, player.nation(), uc_treasure,
          world_square );
      result = LostCityRumorResult::unit_created{ .id = id };
      break;
    }
    case e_burial_mounds_type::cold_and_empty: {
      co_await gui.message_box(
          "The mounds are cold and empty." );
      result = LostCityRumorResult::other{};
      break;
    }
  }
  if( has_burial_grounds ) {
    co_await gui.message_box(
        "These are native burial grounds.  WAR!" );
  }
  co_return result;
}

e_unit_type pick_unit_type_for_foy() {
  // FIXME: need to move this to the immigration module and have
  // a queue located in a new top-level save-game state repre-
  // senting the old world state.
  refl::enum_map<e_unit_type, int> weights{
      { e_unit_type::petty_criminal, 10 },         //
      { e_unit_type::indentured_servant, 10 },     //
      { e_unit_type::free_colonist, 10 },          //
      { e_unit_type::soldier, 10 },                //
      { e_unit_type::dragoon, 10 },                //
      { e_unit_type::pioneer, 10 },                //
      { e_unit_type::missionary, 10 },             //
      { e_unit_type::scout, 10 },                  //
      { e_unit_type::expert_farmer, 10 },          //
      { e_unit_type::expert_fisherman, 10 },       //
      { e_unit_type::expert_sugar_planter, 10 },   //
      { e_unit_type::expert_tobacco_planter, 10 }, //
      { e_unit_type::expert_cotton_planter, 10 },  //
      { e_unit_type::expert_fur_trapper, 10 },     //
      { e_unit_type::expert_lumberjack, 10 },      //
      { e_unit_type::expert_ore_miner, 10 },       //
      { e_unit_type::expert_silver_miner, 10 },    //
      { e_unit_type::master_carpenter, 10 },       //
      { e_unit_type::master_rum_distiller, 10 },   //
      { e_unit_type::master_tobacconist, 10 },     //
      { e_unit_type::master_weaver, 10 },          //
      { e_unit_type::master_fur_trader, 10 },      //
      { e_unit_type::master_blacksmith, 10 },      //
      { e_unit_type::master_gunsmith, 10 },        //
      { e_unit_type::elder_statesman, 10 },        //
      { e_unit_type::firebrand_preacher, 10 },     //
      { e_unit_type::hardy_colonist, 10 },         //
      { e_unit_type::jesuit_colonist, 10 },        //
      { e_unit_type::seasoned_colonist, 10 },      //
      { e_unit_type::veteran_colonist, 10 },       //
      { e_unit_type::veteran_soldier, 10 },        //
      { e_unit_type::veteran_dragoon, 10 },        //
      { e_unit_type::hardy_pioneer, 10 },          //
      { e_unit_type::jesuit_missionary, 10 },      //
      { e_unit_type::seasoned_scout, 10 },         //
  };
  return rng::pick_from_weighted_enum_values( weights );
}

wait<LostCityRumorResult_t> run_rumor_result(
    e_rumor_type type, e_burial_mounds_type burial_type,
    bool has_burial_grounds, UnitsState& units_state, IGui& gui,
    Player& player, IMapUpdater& map_updater, UnitId unit_id,
    Coord world_square ) {
  e_lcr_explorer_category const explorer =
      lcr_explorer_category( units_state, unit_id );
  switch( type ) {
    case e_rumor_type::none: {
      co_await gui.message_box( "You find nothing but rumors." );
      co_return LostCityRumorResult::other{};
    }
    case e_rumor_type::fountain_of_youth: {
      co_await gui.message_box(
          "You've discovered a Fountain of Youth!" );
      int const count =
          config_lcr.fountain_of_youth_num_immigrants;
      // TODO: we need to store a queue of immigrants so that the
      // list displayed is consistent with what is currently
      // shown in the old world.
      for( int i = 0; i < count; ++i ) {
        e_unit_type next = pick_unit_type_for_foy();
        UnitId      id =
            create_unit( units_state, player.nation(),
                         UnitComposition::create( next ) );
        units_state.change_to_harbor_view(
            id, UnitHarborViewState::in_port{} );
        co_await gui.message_box(
            "A @[H]{}@[] has arrived in port!",
            units_state.unit_for( id ).desc().name );
      }
      co_return LostCityRumorResult::other{};
    }
    case e_rumor_type::ruins: {
      int amount = random_gift(
          { .min      = config_lcr.ruins_gift_min[explorer],
            .max      = config_lcr.ruins_gift_max[explorer],
            .multiple = config_lcr.ruins_gift_multiple } );
      co_await gui.message_box(
          "You've discovered the ruins of a lost colony, among "
          "which there are items worth @[H]{}@[] in gold.",
          amount );
      int total = player.add_money( amount );
      lg.info(
          "{} gold added to {} treasury.  current balance: {}.",
          amount, player.nation(), total );
      co_return LostCityRumorResult::other{};
    }
    case e_rumor_type::burial_mounds: {
      ui::e_confirm res = co_await gui.yes_no(
          { .msg = "You stumble across some mysterious ancient "
                   "burial mounds.  Explore them?",
            .yes_label      = "Let us search for treasure!",
            .no_label       = "Leave them alone.",
            .no_comes_first = false } );
      if( res == ui::e_confirm::no )
        co_return LostCityRumorResult::other{};
      LostCityRumorResult_t result =
          co_await run_burial_mounds_result(
              burial_type, has_burial_grounds, units_state, gui,
              player, map_updater, unit_id, world_square );
      co_return result;
    }
    case e_rumor_type::chief_gift: {
      int amount = random_gift(
          { .min      = config_lcr.chief_gift_min[explorer],
            .max      = config_lcr.chief_gift_max[explorer],
            .multiple = config_lcr.chief_gift_multiple } );
      co_await gui.message_box(
          "You happen upon a small village.  The chief offers "
          "you a gift worth @[H]{}@[] gold.",
          amount );
      int total = player.add_money( amount );
      lg.info(
          "{} gold added to {} treasury.  current balance: {}.",
          amount, player.nation(), total );
      co_return LostCityRumorResult::other{};
    }
    case e_rumor_type::free_colonist: {
      co_await gui.message_box(
          "You happen upon the survivors of a lost colony.  In "
          "exchange for badly-needed supplies, they agree to "
          "swear allegiance to you and join your expedition." );
      // Use the non-coroutine version of this because it avoids
      // an inifinite-regress issue where the new unit created
      // rediscovers the LCR on this tile; also, there are no
      // further UI actions needed in response to creating this
      // unit, apart from what we will do here.
      UnitId id = create_unit_on_map_no_ui(
          units_state, map_updater, player.nation(),
          UnitComposition::create(
              UnitType::create( e_unit_type::free_colonist ) ),
          world_square );
      co_return LostCityRumorResult::unit_created{ .id = id };
    }
    case e_rumor_type::unit_lost: {
      // Destroy unit before showing message so that the unit ac-
      // tually appears to disappear.
      units_state.destroy_unit( unit_id );
      co_await gui.message_box(
          "Our colonist has vanished without a trace." );
      co_return LostCityRumorResult::unit_lost{};
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
    default: return e_lcr_explorer_category::other;
  }
}

e_burial_mounds_type pick_burial_mounds_result(
    e_lcr_explorer_category explorer ) {
  refl::enum_map<e_burial_mounds_type, int> const& weights =
      config_lcr.burial_mounds_type_weights[explorer];
  return rng::pick_from_weighted_enum_values( weights );
}

e_rumor_type pick_rumor_type_result(
    e_lcr_explorer_category explorer, Player const& player ) {
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

  return rng::pick_from_weighted_enum_values( weights );
}

bool pick_burial_grounds_result(
    Player const& player, e_lcr_explorer_category explorer,
    e_burial_mounds_type burial_type ) {
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
  return rng::flip_coin(
      config_lcr.burial_grounds_probability[explorer] );
}

wait<LostCityRumorResult_t> run_lost_city_rumor_result(
    UnitsState& units_state, IGui& gui, Player& player,
    IMapUpdater& map_updater, UnitId unit_id, Coord world_square,
    e_rumor_type type, e_burial_mounds_type burial_type,
    bool has_burial_grounds ) {
  LostCityRumorResult_t result = co_await run_rumor_result(
      type, burial_type, has_burial_grounds, units_state, gui,
      player, map_updater, unit_id, world_square );

  // Remove lost city rumor.
  map_updater.modify_map_square(
      world_square, []( MapSquare& square ) {
        CHECK_EQ( square.lost_city_rumor, true );
        square.lost_city_rumor = false;
      } );

  co_return result;
}

} // namespace rn
