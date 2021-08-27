/****************************************************************
**ext-std.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-25.
*
* Description: Rcl extensions for standard library types.
*
*****************************************************************/
#include "ext-std.hpp"

// base
#include "base/maybe.hpp"

using namespace std;

namespace rcl {

convert_err<string> convert_to( value const& v, tag<string> ) {
  base::maybe<string const&> s = v.get_if<string>();
  if( !s.has_value() )
    return error( fmt::format(
        "cannot convert value of type {} to string.",
        name_of( type_of( v ) ) ) );
  return *s;
}

convert_err<string_view> convert_to(
    value const& v ATTR_LIFETIMEBOUND, tag<string_view> ) {
  base::maybe<string const&> s = v.get_if<string>();
  if( !s.has_value() )
    return error( fmt::format(
        "cannot convert value of type {} to string_view.",
        name_of( type_of( v ) ) ) );
  return string_view( *s );
}

convert_err<fs::path> convert_to( value const& v,
                                  tag<fs::path> ) {
  base::maybe<string const&> s = v.get_if<string>();
  if( !s.has_value() )
    return error( fmt::format(
        "cannot convert value of type {} to file path.",
        name_of( type_of( v ) ) ) );
  return fs::path( *s );
}

convert_err<chrono::seconds> convert_to( value const& v,
                                         tag<chrono::seconds> ) {
  base::maybe<int const&> i = v.get_if<int>();
  if( !i.has_value() )
    return error( fmt::format(
        "cannot convert value of type {} to chrono::seconds.",
        name_of( type_of( v ) ) ) );
  return chrono::seconds{ *i };
}

} // namespace rcl
