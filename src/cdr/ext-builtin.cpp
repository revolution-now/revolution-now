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

// cdr
#include "converter.hpp"

using namespace std;

namespace cdr {

/****************************************************************
** int
*****************************************************************/
value to_canonical( converter&, int o, tag_t<int> ) {
  return value{ integer_type{ o } };
}

result<int> from_canonical( converter& conv, value const& v,
                            tag_t<int> ) {
  if( !v.holds<integer_type>() )
    return conv.err(
        "failed to convert value of type {} to int.",
        type_name( v ) );
  return v.get<integer_type>();
}

/****************************************************************
** bool
*****************************************************************/
value to_canonical( converter&, bool o, tag_t<bool> ) {
  return value{ o };
}

result<bool> from_canonical( converter& conv, value const& v,
                             tag_t<bool> ) {
  if( !v.holds<bool>() )
    return conv.err(
        "failed to convert value of type {} to bool.",
        type_name( v ) );
  return v.get<bool>();
}

/****************************************************************
** double
*****************************************************************/
value to_canonical( converter&, double o, tag_t<double> ) {
  return value{ o };
}

result<double> from_canonical( converter& conv, value const& v,
                               tag_t<double> ) {
  if( !v.holds<double>() )
    return conv.err(
        "failed to convert value of type {} to double.",
        type_name( v ) );
  return v.get<double>();
}

} // namespace cdr
