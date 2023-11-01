/****************************************************************
**binary.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-01.
*
* Description: TODO [FILL ME IN]
*
*****************************************************************/
#include "binary.hpp"

using namespace std;

namespace sav {

namespace {

using ::base::valid;
using ::base::valid_or;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
valid_or<string> load_binary( string const& path,
                              ColonySAV&    out ) {
  return valid;
}

valid_or<string> sav_binary( string const&    path,
                             ColonySAV const& in ) {
  return valid;
}

} // namespace sav
