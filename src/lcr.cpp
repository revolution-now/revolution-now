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
#include "gs-events.hpp"
#include "gs-terrain.hpp"
#include "gs-units.hpp"
#include "logger.hpp"
#include "old-world.hpp"
#include "rand-enum.hpp"
#include "rand.hpp"
#include "ustate.hpp"
#include "utype.hpp"
#include "window.hpp"

// config
#include "config/lcr.rds.hpp"

// Rds
#include "lcr-impl.rds.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

struct GiftOptions {
  int min      = 0;
  int max      = 0;
  int multiple = 10;
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

bool has_hernando_de_soto() {
  // TODO
  return false;
}

// When exploring burial mounds that are in native owned land we
// could find indian burial grounds, which will cause that tribe
// to declare war (permanently?).
bool has_burial_grounds() {
  // TODO
  return false;
}

// The fountain of youth is only allowed pre-independence.
bool allow_fountain_of_youth( EventsState const& events_state ) {
  return !events_state.independence_declared;
}

wait<> run_burial_mounds_result( e_burial_mounds_type type,
                                 UnitsState&  units_state,
                                 Player&      player,
                                 IMapUpdater& map_updater,
                                 UnitId       unit_id,
                                 Coord        world_square ) {
  bool positive_result = {};
  switch( type ) {
    case e_burial_mounds_type::trinkets: {
      int amount =
          random_gift( { .min = config_lcr.trinkets_gift_min,
                         .max = config_lcr.trinkets_gift_max,
                         .multiple = 10 } );
      co_await ui::message_box(
          "You've find some trinkets worth @[H]{}@[] gold.",
          amount );
      int total = player.add_money( amount );
      lg.info(
          "{} gold added to {} treasury.  current balance: {}.",
          amount, player.nation(), total );
      positive_result = true;
      break;
    }
    case e_burial_mounds_type::treasure_train: {
      int amount = random_gift(
          { .min      = config_lcr.lost_city_treasure_min,
            .max      = config_lcr.lost_city_treasure_max,
            .multiple = 100 } );
      co_await ui::message_box(
          "You've discovered the seven cities of Cibola, and "
          "have recovered a treasure worth @[H]{}@[].",
          amount );
      UNWRAP_CHECK(
          uc_treasure,
          UnitComposition::create(
              UnitType::create( e_unit_type::large_treasure ),
              { { e_unit_inventory::gold, amount } } ) );
      create_unit_on_map( units_state, map_updater,
                          player.nation(), uc_treasure,
                          world_square );
      positive_result = true;
      break;
    }
    case e_burial_mounds_type::cold_and_empty: {
      co_await ui::message_box(
          "The mounds are cold and empty." );
      positive_result = false;
      break;
    }
  }

  // TODO: Some say that suppressing burial grounds via De Soto
  // requires using a scout (need to determine this). Also, some
  // say that even with De Soto you can still stumble on native
  // burial grounds, though that would always be accompanied by a
  // positive result otherwise such as a treasure, hence the
  // logic below. In that way, De Soto only means that "purely
  // negative" results are prevented. But this needs to be deter-
  // mined.
  bool allow_burial_grounds =
      !has_hernando_de_soto() || positive_result;
  if( has_burial_grounds() && allow_burial_grounds ) {
    co_await ui::message_box(
        "These are native burial grounds.  WAR!" );
  }
}

e_burial_mounds_type pick_burial_mounds_result() {
  EnumMap<e_burial_mounds_type, int> weights =
      config_lcr.burial_mounds_type_weights;
  // TODO: modify weights depending on game state.
  return rng::pick_from_weighted_enum_values( weights );
}

e_unit_type pick_unit_type_for_foy() {
  // TODO: need to adjust these weights.
  //
  // FIXME: need to move this to the immigration module and have
  // a queue located in a new top-level save-game state repre-
  // senting the old world state.
  EnumMap<e_unit_type, int> weights{
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

wait<> run_rumor_result( e_rumor_type        type,
                         TerrainState const& terrain_state,
                         UnitsState& units_state, Player& player,
                         IMapUpdater& map_updater,
                         UnitId unit_id, Coord world_square ) {
  switch( type ) {
    case e_rumor_type::none: {
      co_await ui::message_box( "You find nothing but rumors." );
      break;
    }
    case e_rumor_type::fountain_of_youth: {
      co_await ui::message_box(
          "You've discovered the Fountain of Youth!" );
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
        units_state.change_to_old_world_view(
            id, UnitOldWorldViewState::in_port{} );
        co_await ui::message_box(
            "A @[H]{}@[] has arrived in port!",
            units_state.unit_for( id ).desc().name );
      }
      break;
    }
    case e_rumor_type::ruins: {
      int amount =
          random_gift( { .min      = config_lcr.ruins_gift_min,
                         .max      = config_lcr.ruins_gift_max,
                         .multiple = 10 } );
      co_await ui::message_box(
          "You've discovered ruins of a lost civilization, "
          "among which there are items worth @[H]{}@[] in gold.",
          amount );
      int total = player.add_money( amount );
      lg.info(
          "{} gold added to {} treasury.  current balance: {}.",
          amount, player.nation(), total );
      break;
    }
    case e_rumor_type::burial_mounds: {
      ui::e_confirm res = co_await ui::yes_no(
          "You stumble across some mysterious ancient burial "
          "mounds.  Explore them?" );
      if( res == ui::e_confirm::no ) break;
      e_burial_mounds_type bm_type = pick_burial_mounds_result();
      co_await run_burial_mounds_result( bm_type, units_state,
                                         player, map_updater,
                                         unit_id, world_square );
      break;
    }
    case e_rumor_type::chief_gift: {
      int amount =
          random_gift( { .min      = config_lcr.chief_gift_min,
                         .max      = config_lcr.chief_gift_max,
                         .multiple = 10 } );
      co_await ui::message_box(
          "You happen upon a small village.  The chief offers "
          "you a gift worth @[H]{}@[] gold.",
          amount );
      int total = player.add_money( amount );
      lg.info(
          "{} gold added to {} treasury.  current balance: {}.",
          amount, player.nation(), total );
      break;
    }
    case e_rumor_type::free_colonist: {
      co_await ui::message_box(
          "You happen upon the survivors of a lost colony.  In "
          "exchange for badly-needed supplies, they agree to "
          "swear allegiance to you and join your expedition." );
      create_unit_on_map(
          units_state, map_updater, player.nation(),
          UnitComposition::create(
              UnitType::create( e_unit_type::free_colonist ) ),
          world_square );
      break;
    }
    case e_rumor_type::unit_lost: {
      // Destroy unit before showing message so that the unit ac-
      // tually appears to disappear.
      units_state.destroy_unit( unit_id );
      co_await ui::message_box(
          "Our colonist has vanished without a trace." );
      break;
    }
    case e_rumor_type::nearby_land: {
      co_await ui::message_box( "Nearby lands?" );
      break;
    }
    case e_rumor_type::scout_upgrade: {
      Unit& unit = units_state.unit_for( unit_id );
      if( rng::flip_coin() &&
          unit.type() == e_unit_type::scout ) {
        if( !config_lcr.scout_upgrades_other_expert_unit ) {
          // We're not upgrading units that already have some
          // other expertise.
          if( unit.desc().expertise.has_value() ) break;
        }
        unit.change_type( UnitComposition::create(
            UnitType::create( e_unit_type::seasoned_scout ) ) );
        co_await ui::message_box( "Scout Upgrade?" );
      }
      break;
    }
  }
  co_return;
}

bool did_kill_unit( e_rumor_type type ) {
  switch( type ) {
    case e_rumor_type::none:
    case e_rumor_type::fountain_of_youth:
    case e_rumor_type::ruins:
    case e_rumor_type::burial_mounds:
    case e_rumor_type::chief_gift:
    case e_rumor_type::free_colonist:
    case e_rumor_type::nearby_land:
    case e_rumor_type::scout_upgrade: return false;
    case e_rumor_type::unit_lost: return true;
  }
}

} // namespace

bool has_lost_city_rumor( TerrainState const& terrain_state,
                          Coord               square ) {
  return terrain_state.square_at( square ).lost_city_rumor;
}

wait<e_lost_city_rumor_result> enter_lost_city_rumor(
    TerrainState const& terrain_state, UnitsState& units_state,
    EventsState const& events_state, Player& player,
    IMapUpdater& map_updater, UnitId unit_id,
    Coord world_square ) {
  EnumMap<e_rumor_type, int> weights =
      config_lcr.rumor_type_weights;

  if( !allow_fountain_of_youth( events_state ) )
    weights[e_rumor_type::fountain_of_youth] = 0;

  if( has_hernando_de_soto() ) {
    // Most docs say that having De Soto will prevent LCR results
    // from being negative.  This means that you can still get
    // neutral results, such as "nothing but rumors", but the
    // solely-negative results are prevented.
    //
    // TODO: However, some say that this requires using a scout
    // (need to determine this). Also, some say that even with De
    // Soto you can still stumble on native burial grounds,
    // though that would always be accompanied by a positive re-
    // sult otherwise such as a treasure. In that way, De Soto
    // only means that "purely negative" results are prevented.
    // But this needs to be determined.
    weights[e_rumor_type::unit_lost] = 0;
  }

  e_rumor_type type =
      rng::pick_from_weighted_enum_values( weights );

  co_await run_rumor_result( type, terrain_state, units_state,
                             player, map_updater, unit_id,
                             world_square );

  // Remove lost city rumor.
  map_updater.modify_map_square(
      world_square, []( MapSquare& square ) {
        CHECK_EQ( square.lost_city_rumor, true );
        square.lost_city_rumor = false;
      } );

  co_return did_kill_unit( type )
      ? e_lost_city_rumor_result::unit_lost
      : e_lost_city_rumor_result::unit_alive;
}

} // namespace rn
