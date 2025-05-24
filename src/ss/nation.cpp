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
e_nation nation_for( e_player const player ) {
  switch( player ) {
    case e_player::english:
      return e_nation::english;
    case e_player::french:
      return e_nation::french;
    case e_player::spanish:
      return e_nation::spanish;
    case e_player::dutch:
      return e_nation::dutch;
    case e_player::ref_english:
      return e_nation::english;
    case e_player::ref_french:
      return e_nation::french;
    case e_player::ref_spanish:
      return e_nation::spanish;
    case e_player::ref_dutch:
      return e_nation::dutch;
  }
}

e_player colonist_player_for( e_nation const nation ) {
  switch( nation ) {
    case e_nation::english:
      return e_player::english;
    case e_nation::french:
      return e_player::french;
    case e_nation::spanish:
      return e_player::spanish;
    case e_nation::dutch:
      return e_player::dutch;
  }
}

e_player ref_player_for( e_nation const nation ) {
  switch( nation ) {
    case e_nation::english:
      return e_player::ref_english;
    case e_nation::french:
      return e_player::ref_french;
    case e_nation::spanish:
      return e_player::ref_spanish;
    case e_nation::dutch:
      return e_player::ref_dutch;
  }
}

bool is_ref( e_player const player ) {
  switch( player ) {
    case e_player::english:
      return false;
    case e_player::french:
      return false;
    case e_player::spanish:
      return false;
    case e_player::dutch:
      return false;
    case e_player::ref_english:
      return true;
    case e_player::ref_french:
      return true;
    case e_player::ref_spanish:
      return true;
    case e_player::ref_dutch:
      return true;
  }
}

} // namespace rn
