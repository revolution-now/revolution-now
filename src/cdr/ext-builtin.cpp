/****************************************************************
**ext-builtin.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-27.
*
* Description: Cdr conversions for builtin types.
*
*****************************************************************/
#include "ext-builtin.hpp"

using namespace std;

namespace cdr {

/****************************************************************
** int
*****************************************************************/
value to_canonical( int o, tag_t<int> ) { return value{ o }; }

result<int> from_canonical( value const& v, tag_t<int> ) {
  if( !v.holds<int>() )
    return error(
        "failed to convert cdr value of type {} to int.",
        type_name( v ) );
  return v.get<int>();
}

/****************************************************************
** bool
*****************************************************************/
value to_canonical( bool o, tag_t<bool> ) { return value{ o }; }

result<bool> from_canonical( value const& v, tag_t<bool> ) {
  if( !v.holds<bool>() )
    return error(
        "failed to convert cdr value of type {} to bool.",
        type_name( v ) );
  return v.get<bool>();
}

/****************************************************************
** double
*****************************************************************/
value to_canonical( double o, tag_t<double> ) {
  return value{ o };
}

result<double> from_canonical( value const& v, tag_t<double> ) {
  if( !v.holds<double>() )
    return error(
        "failed to convert cdr value of type {} to double.",
        type_name( v ) );
  return v.get<double>();
}

} // namespace cdr
