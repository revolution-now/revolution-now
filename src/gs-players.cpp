/****************************************************************
**gs-players.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-13.
*
* Description: Serializable player-related state.
*
*****************************************************************/
#include "gs-players.hpp"

using namespace std;

namespace rn {

base::valid_or<string> PlayersState::validate() const {
  // Check that players have the correct nation relative to their
  // key in the map.
  for( auto const& [nation, player] : players )
    REFL_VALIDATE( player.nation() == nation,
                   "mismatch in player nations." );
  return base::valid;
}

} // namespace rn
