/****************************************************************
**ext-builtin.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-25.
*
* Description: Rcl extensions for builtin types.
*
*****************************************************************/
#include "ext-builtin.hpp"

// base
#include "base/maybe.hpp"

using namespace std;

namespace rcl {

convert_err<int> convert_to( value const& v, tag<int> ) {
  base::maybe<int const&> i = v.get_if<int>();
  if( !i.has_value() )
    return error(
        fmt::format( "cannot convert value of type {} to int.",
                     name_of( type_of( v ) ) ) );
  return *i;
}

convert_err<bool> convert_to( value const& v, tag<bool> ) {
  base::maybe<bool const&> b = v.get_if<bool>();
  if( !b.has_value() )
    return error(
        fmt::format( "cannot convert value of type {} to bool.",
                     name_of( type_of( v ) ) ) );
  return *b;
}

convert_err<double> convert_to( value const& v, tag<double> ) {
  base::maybe<double const&> d = v.get_if<double>();
  if( d.has_value() ) return *d;
  base::maybe<int const&> i = v.get_if<int>();
  if( i.has_value() ) return *i;
  return error(
      fmt::format( "cannot convert value of type {} to double.",
                   name_of( type_of( v ) ) ) );
}

} // namespace rcl
