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
#include "unit.hpp"

namespace rn {

namespace {



} // namespace

k_turn_result turn() {
  //start of turn:

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

  auto gave_orders{false};

  //  Iterate through all units, for each:
  for( auto unit_id : units_all( player_nationality() ) ) {
    auto const& u = unit_from_id( unit_id );
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
    //if( u.orders == g_unit_orders::none ) {
    //  gave_orders = true;
    //  loop_orders( unit_id );
    //}

    //    * Make AI moves
    //        Make European moves
    //        Make Native moves
    //        Make expeditionary force moves
    //      TODO

    //    * if no player units needed orders then show a message somewhere
    //      that says "end of turn" and let the user interact with the
    //      map and GUI.
    if( !gave_orders ) {
      switch( loop_eot() ) {
        case k_loop_result::quit:
          return k_turn_result::quit;
          break;
        case k_loop_result::none:
          break;
      };
    }
  }
  return k_turn_result::cont;
}

} // namespace rn
