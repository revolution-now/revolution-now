/****************************************************************
**isfx.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-30.
*
* Description: Interface for sound effects.
*
*****************************************************************/
#include "isfx.hpp"

using namespace std;

namespace sfx {

namespace {} // namespace

void to_str( error const& o, string& out, base::tag<error> ) {
  base::to_str(
      fmt::format( "{}:{}: error: {}", o.loc.file_name(),
                   o.loc.line(), o.msg ),
      out );
}

} // namespace sfx
