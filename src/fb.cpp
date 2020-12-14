/****************************************************************
**fb.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-19.
*
* Description: Stuff needed by headers with serializable types.
*
*****************************************************************/
#include "fb.hpp"

// Abseil
#include "absl/strings/str_replace.h"

// C++ standard library
#include <string_view>

using namespace std;

namespace rn::serial {

/****************************************************************
** Public API
*****************************************************************/
string ns_to_dots( string_view sv ) {
  return absl::StrReplaceAll( sv, { { "::", "." } } );
}

} // namespace rn::serial
