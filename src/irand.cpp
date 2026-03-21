/****************************************************************
**irand.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-11.
*
* Description: Injectable interface for random number generation.
*
*****************************************************************/
#include "irand.hpp"

using namespace std;

namespace rn {

/****************************************************************
** IRand
*****************************************************************/
void to_str( IRand const& o, string& out, base::tag<IRand> ) {
  out += "IRand@";
  out += fmt::format( "{}", static_cast<void const*>( &o ) );
}

} // namespace rn
