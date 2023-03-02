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
          if( maybe<e_nation> const human =
                  player_for_role( ss, e_player_role::human );
              human.has_value() )
            return human;
          return player_for_role( ss, e_player_role::active );
        }
        CASE( entire ) { //
          return nothing;
        }
        CASE( nation ) { return o.nation; }
        END_CASES;
      }
    }
    case e_player_role::human:
      return ss.players.human;
    case e_player_role::active:
      if( auto nations =
              ss.turn.cycle.get_if<TurnCycle::nations>();
          nations.has_value() )
        for( e_nation const nation :
             refl::enum_values<e_nation> )
          if( nations->which[nation].to_enum() >
                  NationTurnState::e::not_started &&
              nations->which[nation].to_enum() <
                  NationTurnState::e::finished )
            return nation;
      return nothing;
  }
}

} // namespace rn
