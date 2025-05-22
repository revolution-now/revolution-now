/****************************************************************
**nation.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-21.
*
* Description: Player/nation enums.
*
*****************************************************************/
#include "nation.hpp"

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
e_european_nation european_nation_for( e_player const player ) {
  switch( player ) {
    case e_player::english:
      return e_european_nation::english;
    case e_player::french:
      return e_european_nation::french;
    case e_player::spanish:
      return e_european_nation::spanish;
    case e_player::dutch:
      return e_european_nation::dutch;
  }
}

e_player colonist_player_for( e_european_nation const nation ) {
  switch( nation ) {
    case e_european_nation::english:
      return e_player::english;
    case e_european_nation::french:
      return e_player::french;
    case e_european_nation::spanish:
      return e_player::spanish;
    case e_european_nation::dutch:
      return e_player::dutch;
  }
}

} // namespace rn
