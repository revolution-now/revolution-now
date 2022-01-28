/****************************************************************
**ext-std.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-28.
*
* Description: Cdr conversions for std types.
*
*****************************************************************/
#include "ext-std.hpp"

using namespace std;

namespace cdr {

/****************************************************************
** string
*****************************************************************/
value to_canonical( string const& o, tag_t<string> ) {
  return o;
}

result<string> from_canonical( value const& v, tag_t<string> ) {
  auto maybe_str = v.get_if<string>();
  if( !maybe_str.has_value() )
    return error::build{ "std::string" }(
        "producing a std::string requires type string, instead "
        "found type {}.",
        type_name( v ) );
  return *maybe_str;
}

/****************************************************************
** string_view
*****************************************************************/
value to_canonical( string_view const& o, tag_t<string_view> ) {
  return string( o );
}

} // namespace cdr
