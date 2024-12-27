/****************************************************************
**ivideo.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-27.
*
* Description: Interface for dealing with the program window.
*
*****************************************************************/
#include "ivideo.hpp"

using namespace std;

namespace vid {

namespace {} // namespace

void to_str( error const& o, string& out, base::tag<error> ) {
  base::to_str(
      fmt::format( "{}:{}: error: {}", o.loc.file_name(),
                   o.loc.line(), o.msg ),
      out );
}

} // namespace vid
