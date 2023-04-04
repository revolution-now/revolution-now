/****************************************************************
**roles.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-01.
*
* Description: Manages the different roles that each player can
*              have with regard to driving the game and map
*              visibility.
*
*****************************************************************/
#include "roles.hpp"

// ss
#include "ss/land-view.rds.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/turn.rds.hpp"

// rds
#include "rds/switch-macro.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API
*****************************************************************/
maybe<e_nation> player_for_role( SSConst const& ss,
                                 e_player_role  role ) {
  switch( role ) {
    case e_player_role::viewer: {
      SWITCH( ss.land_view.map_revealed ) {
        CASE( no_special_view ) {
          maybe<e_nation> const active =
              player_for_role( ss, e_player_role::active );
          if( active.has_value() && ss.players.humans[*active] )
            return *active;
          // Find the first human that we can find. In a normal
          // game, where there is only one human player, this
          // will yield that one. If there are multiple human
          // players then we are just picking the first one. This
          // is a bit arbitrary, but there doesn't seem to be
          // anything better to do here.
          for( e_nation nation : refl::enum_values<e_nation> )
            if( ss.players.humans[nation] ) return nation;
          return active;
        }
        CASE( entire ) { //
          return nothing;
        }
        CASE( nation ) { return nation.nation; }
        END_CASES;
      }
      SHOULD_NOT_BE_HERE; // for gcc.
    }
    case e_player_role::active:
      return ss.turn.cycle.get_if<TurnCycle::nation>().member(
          &TurnCycle::nation::nation );
  }
}

} // namespace rn
