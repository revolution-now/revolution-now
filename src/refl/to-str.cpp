/****************************************************************
**to-str.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-10.
*
* Description: to_str extension for reflected types.
*
*****************************************************************/
#include "to-str.hpp"

using namespace std;

namespace refl {

namespace detail {

string build_ns_prefix_from_refl_ns( string_view ns ) {
  string res;
  if( ns.empty() ) return res;
  res = string( ns ) + "::";
  if( res.starts_with( "rn::" ) )
    res = string( res.begin() + 4, res.end() );
  return res;
}

} // namespace detail

} // namespace refl
