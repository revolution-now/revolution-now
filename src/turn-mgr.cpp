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
#include "ss/nation.hpp"
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
maybe<e_nation> find_first_nation_to_move( SSConst const& ss ) {
  auto const& ns = refl::enum_values<e_nation>;
  for( e_nation const nation : ns )
    if( ss.players.players[colonist_player_for( nation )]
            .has_value() ) //
      return nation;
  return nothing;
}

// NOTE: should allow that no nations exist here.
maybe<e_nation> find_next_nation_to_move(
    SSConst const& ss, e_nation const curr_nation ) {
  auto const& ns = refl::enum_values<e_nation>;

  // Find the current nation.
  auto it = ns.begin();
  while( it != ns.end() && *it != curr_nation ) ++it;
  CHECK( it != ns.end() );
  CHECK_EQ( *it, curr_nation );

  // Find the next one that exits.
  ++it;
  for( ; it != ns.end(); ++it ) {
    e_nation const nation = *it;
    if( ss.players.players[colonist_player_for( nation )]
            .has_value() ) //
      return nation;
  }

  return nothing;
}

} // namespace rn
