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
e_european_nation european_nation_for( e_nation const nation ) {
  switch( nation ) {
    case e_nation::english:
      return e_european_nation::english;
    case e_nation::french:
      return e_european_nation::french;
    case e_nation::spanish:
      return e_european_nation::spanish;
    case e_nation::dutch:
      return e_european_nation::dutch;
  }
}

} // namespace rn
