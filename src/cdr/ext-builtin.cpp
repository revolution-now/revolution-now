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
** char
*****************************************************************/
value to_canonical( converter&, char o, tag_t<char> ) {
  return value{ string( 1, o ) };
}

result<char> from_canonical( converter& conv, value const& v,
                             tag_t<char> ) {
  auto char_str = conv.ensure_type<string>( v );
  if( !char_str.has_value() )
    return conv.err(
        "cannot convert value of type {} to character.",
        type_name( v ) );
  if( char_str->size() != 1 )
    return conv.err(
        "expected character but found string of length {}.",
        char_str->size() );
  return ( *char_str )[0];
}

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
  if( auto d = v.get_if<double>(); d.has_value() ) return *d;
  if( auto n = v.get_if<integer_type>(); n.has_value() )
    return static_cast<double>( *n );
  return conv.err(
      "failed to convert value of type {} to double.",
      type_name( v ) );
}

} // namespace cdr
