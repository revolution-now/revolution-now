/****************************************************************
* turn.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: Main loop that processes a turn.
*
*****************************************************************/
#include "turn.hpp"

#include "loops.hpp"
#include "movement.hpp"
#include "ownership.hpp"
#include "unit.hpp"

#include <algorithm>
#include <queue>

namespace rn {

namespace {



} // namespace

e_turn_result turn() {
  //start of turn:

  // Mark all units as not having moved.
  reset_moves();

  //  Iterate through the colonies, for each:
  //  TODO

  //    * advance state of the colony

  //    * display messages to user any/or show animations where necessary

  //    * allow them to enter colony when events happens; in that case
  //      go to the colony screen game loop.  When the user exits the
  //      colony screen then this colony iteration immediately proceeds;
  //      i.e., user cannot enter any other colonies.  This prevents the
  //      user from making last-minute changes to colonies that have not
  //      yet been advanced in this turn (otherwise that might allow
  //      cheating in some way).

  //    * during this time, the user is not free to scroll
  //      map (menus?) or make any changes to units.  They are also
  //      not allowed to enter colonies apart from the one that has
  //      just been processed.

  //  Advance the state of the old world, possibly displaying messages
  //  to the user where necessary.

  auto need_eot_loop{true};

  // We keep looping until all units that need moving have moved.
  // We don't know this list a priori because some units may de-
  // cide to require orders during the course of this process,
  // and this could happen for various reasons. Perhaps even
  // units could be created during this process (?).
  while( true ) {
    auto units = units_all( player_nationality() );
    auto finished = []( UnitId id ){
      return unit_from_id( id ).finished_turn();
    };
    if( all_of( units.begin(), units.end(), finished ) )
      break;

    std::queue<UnitId> q;
    for( auto id : units )
      q.push( id );

    //  Iterate through all units, for each:
    while( !q.empty() ) {
      auto& unit = unit_from_id( q.front() );
      if( unit.finished_turn() ) {
        q.pop();
        continue;
      }
      // By default, we assume that the processing for the unit
      // this turn will be completed in this loop iteration. In
      // certain cases this will not happen, such as e.g. a unit
      // given a 'wait' command. In that case this variable will
      // be set to false.
      bool will_finish_turn = true;

      //    * if it is it in `goto` mode focus on it and advance it
      //      TODO
      //    * if it is a ship on the high seas then advance it
      //        if it has arrived in the old world then jump to the old world
      //        screen (maybe ask user whether they want to ignore),
      //        which has its own game loop (see old-world loop).
      //      TODO
      //    * if it is in the old world then ignore it, or possibly remind
      //      the user it is there.
      //      TODO
      //    * if it is performing an action, such as building a road,
      //      advance the state.  If it finishes then mark it as active
      //      so that it will wait for orders in the next step.
      //      TODO
      //    * if it is in an indian village then advance it, and mark
      //      it active if it is finished.
      //      TODO

      //    * if unit is waiting for orders then focus on it, and enter
      //      a realtime game loop where the user can interact with the
      //      map and GUI in general.  See `unit orders` game loop.
      while( unit.orders_mean_input_required() &&
             !unit.moved_this_turn() ) {
        need_eot_loop = false;
        // `prioritize` is a function that should be called if it
        // is decided (in the loop_orders function) that a unit
        // needs to be bumped to the front of the queue for ac-
        // cepting orders. Note that it should hurt if the unit
        // is already in the queue, since this turn code will
        // never move a unit after it has already completed its
        // turn, no matter how many times it appears in the
        // queue.
        auto prioritize = [&q]( UnitId id ) { q.push( id ); };
        e_orders_loop_result res = loop_orders( unit.id(), prioritize );
        if( res == e_orders_loop_result::wait ) {
          will_finish_turn = false;
          q.push( q.front() );
          q.pop();
          break;
        }
        if( res == e_orders_loop_result::quit )
          return e_turn_result::quit;
      }
      if( will_finish_turn )
        unit.finish_turn();
    }
  }

  //    * Make AI moves
  //        Make European moves
  //        Make Native moves
  //        Make expeditionary force moves
  //      TODO

  //    * if no player units needed orders then show a message somewhere
  //      that says "end of turn" and let the user interact with the
  //      map and GUI.
  if( need_eot_loop ) {
    switch( loop_eot() ) {
      case e_eot_loop_result::quit:
        return e_turn_result::quit;
      case e_eot_loop_result::none:
        break;
    };
  }
  return e_turn_result::cont;
}

} // namespace rn
