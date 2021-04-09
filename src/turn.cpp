/****************************************************************
**turn.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: Main loop that processes a turn.
*
*****************************************************************/
#include "turn.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "colony-mgr.hpp"
#include "cstate.hpp"
#include "dispatch.hpp"
#include "fb.hpp"
#include "flat-deque.hpp"
#include "flat-queue.hpp"
#include "frame.hpp"
#include "land-view.hpp"
#include "logging.hpp"
#include "panel.hpp" // FIXME
#include "sg-macros.hpp"
#include "sound.hpp"
#include "unit.hpp"
#include "ustate.hpp"
#include "viewport.hpp"
#include "waitable-coro.hpp"
#include "window.hpp"

// base
#include "base/lambda.hpp"

// Flatbuffers
#include "fb/sg-turn_generated.h"

// base-util
#include "base-util/algo.hpp"

// C++ standard library
#include <algorithm>

using namespace std;

namespace rn {

DECLARE_SAVEGAME_SERIALIZERS( Turn );

namespace {

/****************************************************************
** Coroutine turn state.
*****************************************************************/
struct NationState {
  NationState() = default;
  NationState( e_nation nat ) : NationState() { nation = nat; }
  valid_deserial_t check_invariants_safe() { return valid; }

  bool operator==( NationState const& ) const = default;

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, NationState,
  ( e_nation,           nation       ),
  ( bool,               started      ),
  ( bool,               did_colonies ),
  ( bool,               did_units    ),
  ( flat_deque<UnitId>, units        ));
  // clang-format on
};

struct TurnState {
  TurnState() = default;

  void new_turn() {
    started   = false;
    need_eot  = true;
    nation    = nothing;
    remainder = {
        e_nation::english, //
        e_nation::french,  //
        e_nation::dutch,   //
        e_nation::spanish  //
    };
  }

  bool operator==( TurnState const& ) const = default;
  valid_deserial_t check_invariants_safe() { return valid; }

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, TurnState,
  ( bool,                 started   ),
  ( bool,                 need_eot  ),
  ( maybe<NationState>,   nation    ),
  ( flat_queue<e_nation>, remainder ));
  // clang-format on
};

/****************************************************************
** Save-Game State
*****************************************************************/
struct SAVEGAME_STRUCT( Turn ) {
  // Fields that are actually serialized.

  // clang-format off
  SAVEGAME_MEMBERS( Turn,
  ( TurnState, turn ));
  // clang-format on

public:
  // Fields that are derived from the serialized fields.

private:
  SAVEGAME_FRIENDS( Turn );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).

    return valid;
  }
  // Called after all modules are deserialized.
  SAVEGAME_VALIDATE() { return valid; }
};
SAVEGAME_IMPL( Turn );

/****************************************************************
** Helpers
*****************************************************************/
bool should_animate_move( TravelAnalysis const& analysis ) {
  CHECK( holds<e_unit_travel_good>( analysis.desc ) );
  auto type = get<e_unit_travel_good>( analysis.desc );
  // TODO: in the case of board_ship we need to make sure that
  // the ship being borded gets rendered on top because there may
  // be a stack of ships in the square, otherwise it will be de-
  // ceiving to the player. This is because when a land unit en-
  // ters a water square it will just automatically pick a ship
  // and board it.
  switch( type ) {
    case e_unit_travel_good::map_to_map: return true;
    case e_unit_travel_good::board_ship: return true;
    case e_unit_travel_good::offboard_ship: return true;
    case e_unit_travel_good::land_fall: return false;
  };
  SHOULD_NOT_BE_HERE;
}

waitable<> do_unit_animation( UnitId              id,
                              PlayerIntent const& intent ) {
  // Default future object that is born ready.
  auto def = make_waitable<>();
  // Kick off animation if needed.
  return overload_visit(
      intent,
      [&]( TravelAnalysis const& val ) {
        if( !should_animate_move( val ) ) return def;
        UNWRAP_CHECK(
            d, val.move_src.direction_to( val.move_target ) );
        return landview_animate_move( id, d );
      },
      [&]( CombatAnalysis const& val ) {
        auto attacker = id;
        UNWRAP_CHECK( defender, val.target_unit );
        UNWRAP_CHECK( stats, val.fight_stats );
        auto const& defender_unit = unit_from_id( defender );
        auto const& attacker_unit = unit_from_id( attacker );
        e_depixelate_anim dp_anim =
            stats.attacker_wins
                ? ( defender_unit.desc().demoted.has_value()
                        ? e_depixelate_anim::demote
                        : e_depixelate_anim::death )
                : ( attacker_unit.desc().demoted.has_value()
                        ? e_depixelate_anim::demote
                        : e_depixelate_anim::death );
        return landview_animate_attack(
            attacker, defender, stats.attacker_wins, dp_anim );
      },
      [&]( auto const& ) { return def; } );
}

// Returns true if the unit needs to ask the user for input.
bool advance_unit( UnitId id ) {
  auto& unit = unit_from_id( id );
  // - if it is it in `goto` mode focus on it and advance it
  //
  // - if it is a ship on the high seas then advance it if it has
  //   arrived in the old world then jump to the old world screen
  //   (maybe ask user whether they want to ignore), which has
  //   its own game loop (see old-world loop).
  //
  // - if it is in the old world then ignore it, or possibly re-
  //   mind the user it is there.
  //
  // - if it is performing an action, such as building a road,
  //   advance the state. If it finishes then mark it as active
  //   so that it will wait for orders in the next step.
  //
  // - if it is in an indian village then advance it, and mark it
  //   active if it is finished.
  //
  // - if unit is waiting for orders then focus on it, make it
  //   blink, and wait for orders.

  if( !is_unit_on_map_indirect( id ) ) {
    // TODO.
    unit.finish_turn();
    return false;
  }

  if( !unit.orders_mean_input_required() ||
      unit.mv_pts_exhausted() ) {
    unit.finish_turn();
    return false;
  }

  // Unit needs to ask for orders.
  return true;
}

waitable<> do_end_of_turn() {
  waitable<> eot = landview_end_of_turn();
  // Need to co_await here instead of just returning because we
  // need to keep the `eot` alive while we're waiting.
  co_await wait_for_eot_button_click();
}

waitable<> do_next_player_input( UnitId              id,
                                 flat_deque<UnitId>* q ) {
  LandViewPlayerInput_t response;
  if( auto maybe_orders = pop_unit_orders( id ) ) {
    response = LandViewPlayerInput::give_orders{
        .orders = *maybe_orders };
  } else {
    lg.debug( "asking orders for: {}", debug_string( id ) );
    SG().turn.need_eot = false;

    response = co_await landview_get_next_input( id );
  }
  switch( response.to_enum() ) {
    using namespace LandViewPlayerInput;
    case e::change_orders: {
      auto& val = response.get<change_orders>();
      // Move some units to the front of the queue.
      for( auto id_to_add : val.add_to_front ) {
        q->push_front( id_to_add );
        unit_from_id( id_to_add ).unfinish_turn();
      }
      // Move some units to the back of the queue.
      for( UnitId id_to_add : val.add_to_back ) {
        q->push_back( id_to_add );
        unit_from_id( id_to_add ).unfinish_turn();
      }
      break;
    }
    // We have some orders for the current unit.
    case e::give_orders: {
      auto& orders = response.get<give_orders>().orders;
      if( orders.holds<orders::wait>() ) {
        q->push_back( id );
        CHECK( q->front() == id );
        q->pop_front();
        break;
      }

      UNWRAP_CHECK( intent, player_intent( id, orders ) );
      if( !co_await confirm_explain( &intent ) ) break;

      co_await do_unit_animation( id, intent );
      affect_orders( intent );

      for( auto id : units_to_prioritize( intent ) ) {
        q->push_front( id );
        unit_from_id( id ).unfinish_turn();
      }
      break;
    }
  }
}

waitable<> do_units_turn() {
  CHECK( SG().turn.nation );
  auto& st = *SG().turn.nation;
  auto& q  = st.units;

  // Initialize.
  if( q.empty() ) {
    auto units = units_all( st.nation );
    util::sort_by_key( units, []( auto id ) { return id._; } );
    // Why do we need this?
    erase_if( units, L( unit_from_id( _ ).finished_turn() ) );
    for( UnitId id : units ) q.push_back( id );
  }

  while( !q.empty() ) {
    lg.debug( "q: {}", q.to_string( 3 ) );
    UnitId id = *q.front();
    // We need this check because units can be added into the
    // queue in this loop by user input.
    if( !unit_exists( id ) ||
        unit_from_id( id ).finished_turn() ) {
      q.pop_front();
      continue;
    }

    bool should_ask = advance_unit( id );
    if( !should_ask ) {
      q.pop_front();
      continue;
    }

    // We have a unit that needs to ask the user for orders. This
    // will open things up to player input not only to the unit
    // that needs orders, but to all units on the map, and will
    // update the queue with any changes that result. This func-
    // tion will return on any player input (e.g. clearing the
    // orders of another unit) and so we might need to circle
    // back to this line a few times in this while loop until we
    // get the order for the unit in question (unless the player
    // activates another unit).
    co_await do_next_player_input( id, &q );
  }
}

void do_colonies_turn() {
  CHECK( SG().turn.nation );
  auto& st = *SG().turn.nation;
  lg.info( "processing colonies for the {}.", st.nation );
  flat_queue<ColonyId> colonies = colonies_all( st.nation );
  while( !colonies.empty() ) {
    ColonyId colony_id = *colonies.front();
    colonies.pop();
    /*co_await*/ evolve_colony_one_turn( colony_id );
  }
}

waitable<> do_nation_turn() {
  CHECK( SG().turn.nation );
  auto& st = *SG().turn.nation;

  // Starting.
  if( !st.started ) {
    print_bar( '-', fmt::format( "[ {} ]", st.nation ) );
    st.started = true;
  }

  // Colonies.
  if( !st.did_colonies ) {
    do_colonies_turn();
    st.did_colonies = true;
  }

  if( !st.did_units ) {
    co_await do_units_turn();
    st.did_units = true;
  }
  CHECK( st.units.empty() );
}

waitable<> do_next_turn_impl() {
  auto& st = SG().turn;

  // Starting.
  if( !st.started ) {
    print_bar( '=', "[ Starting Turn ]" );
    map_units( []( Unit& unit ) { unit.new_turn(); } );
    st.new_turn();
    st.started = true;
  }

  // Body.
  if( st.nation.has_value() ) {
    co_await do_nation_turn();
    st.nation.reset();
  }

  while( !st.remainder.empty() ) {
    st.nation = NationState( *st.remainder.front() );
    st.remainder.pop();
    co_await do_nation_turn();
    st.nation.reset();
  }

  // Ending.
  if( st.need_eot ) co_await do_end_of_turn();

  st.new_turn();
}

} // namespace

/****************************************************************
** Turn State Advancement
*****************************************************************/
waitable<> do_next_turn() { return do_next_turn_impl(); }

} // namespace rn
