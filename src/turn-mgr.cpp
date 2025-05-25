/****************************************************************
**turn-mgr.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-04-20.
*
* Description: Helpers for turn processing.
*
*****************************************************************/
#include "turn-mgr.hpp"

// ss
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
// NOTE: should allow that no nations exist here.
maybe<e_player> find_first_player_to_move( SSConst const& ss ) {
  auto const& ns = refl::enum_values<e_player>;
  for( e_player const player : ns )
    if( ss.players.players[player].has_value() ) //
      return player;
  return nothing;
}

// NOTE: should allow that no players exist here.
maybe<e_player> find_next_player_to_move(
    SSConst const& ss, e_player const curr_player ) {
  auto const& ns = refl::enum_values<e_player>;

  // Find the current player.
  auto it = ns.begin();
  while( it != ns.end() && *it != curr_player ) ++it;
  CHECK( it != ns.end() );
  CHECK_EQ( *it, curr_player );

  // Find the next one that exists.
  ++it;
  for( ; it != ns.end(); ++it ) {
    e_player const player = *it;
    if( ss.players.players[player].has_value() ) //
      return player;
  }

  return nothing;
}

} // namespace rn
