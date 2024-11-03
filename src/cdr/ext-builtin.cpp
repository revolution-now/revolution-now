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
  if( auto i = v.get_if<integer_type>(); i.has_value() ) {
    if( *i < numeric_limits<char>::min() ||
        *i > numeric_limits<char>::max() )
      return conv.err(
          "received out-of-range integral representation of "
          "char: {}",
          *i );
    return static_cast<char>( *i );
  } else if( auto s = v.get_if<string>(); s.has_value() ) {
    if( s->size() != 1 )
      return conv.err(
          "expected character but found string of length {}.",
          s->size() );
    return ( *s )[0];
  } else {
    return conv.err(
        "cannot convert value of type {} to character.",
        type_name( v ) );
  }
}

/****************************************************************
** int8_t
*****************************************************************/
value to_canonical( converter&, int8_t o, tag_t<int8_t> ) {
  return value{ integer_type{ o } };
}

result<int8_t> from_canonical( converter& conv, value const& v,
                               tag_t<int8_t> ) {
  if( !v.holds<integer_type>() )
    return conv.err(
        "failed to convert value of type {} to int8_t.",
        type_name( v ) );
  integer_type const itype = v.get<integer_type>();
  if( itype < -128 || itype > 127 )
    return conv.err(
        "number out of range for conversion to signed 8 bit "
        "integer: {}",
        itype );
  return static_cast<int8_t>( itype );
}

/****************************************************************
** int16_t
*****************************************************************/
value to_canonical( converter&, int16_t o, tag_t<int16_t> ) {
  return value{ integer_type{ o } };
}

result<int16_t> from_canonical( converter& conv, value const& v,
                                tag_t<int16_t> ) {
  if( !v.holds<integer_type>() )
    return conv.err(
        "failed to convert value of type {} to int16_t.",
        type_name( v ) );
  integer_type const itype = v.get<integer_type>();
  if( itype < -32768 || itype > 32767 )
    return conv.err(
        "number out of range for conversion to signed 16 bit "
        "integer: {}",
        itype );
  return static_cast<int16_t>( itype );
}

/****************************************************************
** int64_t
*****************************************************************/
value to_canonical( converter&, int64_t o, tag_t<int64_t> ) {
  return value{ integer_type{ o } };
}

result<int64_t> from_canonical( converter& conv, value const& v,
                                tag_t<int64_t> ) {
  if( !v.holds<integer_type>() )
    return conv.err(
        "failed to convert value of type {} to int64_t.",
        type_name( v ) );
  return v.get<integer_type>();
}

/****************************************************************
** uint8_t
*****************************************************************/
value to_canonical( converter&, uint8_t o, tag_t<uint8_t> ) {
  return value{ integer_type{ o } };
}

result<uint8_t> from_canonical( converter& conv, value const& v,
                                tag_t<uint8_t> ) {
  if( !v.holds<integer_type>() )
    return conv.err(
        "failed to convert value of type {} to uint8_t.",
        type_name( v ) );
  integer_type const itype = v.get<integer_type>();
  if( itype < 0 || itype > 255 )
    return conv.err(
        "number out of range for conversion to unsigned 8 bit "
        "integer: {}",
        itype );
  return static_cast<uint8_t>( itype );
}

/****************************************************************
** uint16_t
*****************************************************************/
value to_canonical( converter&, uint16_t o, tag_t<uint16_t> ) {
  return value{ integer_type{ o } };
}

result<uint16_t> from_canonical( converter& conv, value const& v,
                                 tag_t<uint16_t> ) {
  if( !v.holds<integer_type>() )
    return conv.err(
        "failed to convert value of type {} to uint16_t.",
        type_name( v ) );
  integer_type const itype = v.get<integer_type>();
  if( itype < 0 || itype > 65535 )
    return conv.err(
        "number out of range for conversion to unsigned 16 bit "
        "integer: {}",
        itype );
  return static_cast<uint16_t>( itype );
}

/****************************************************************
** uint32_t
*****************************************************************/
value to_canonical( converter&, uint32_t o, tag_t<uint32_t> ) {
  return value{ integer_type{ o } };
}

result<uint32_t> from_canonical( converter& conv, value const& v,
                                 tag_t<uint32_t> ) {
  if( !v.holds<integer_type>() )
    return conv.err(
        "failed to convert value of type {} to uint32_t.",
        type_name( v ) );
  integer_type const itype = v.get<integer_type>();
  if( itype < 0 || itype > 4294967295 )
    return conv.err(
        "number out of range for conversion to unsigned 32 bit "
        "integer: {}",
        itype );
  return static_cast<uint32_t>( itype );
}

/****************************************************************
** uint64_t
*****************************************************************/
value to_canonical( converter&, uint64_t o, tag_t<uint64_t> ) {
  // Unfortunately this is lossy...
  return value{ integer_type{ static_cast<int64_t>( o ) } };
}

result<uint64_t> from_canonical( converter& conv, value const& v,
                                 tag_t<uint64_t> ) {
  if( !v.holds<integer_type>() )
    return conv.err(
        "failed to convert value of type {} to uint64_t.",
        type_name( v ) );
  integer_type const itype = v.get<integer_type>();
  if( itype < 0 )
    return conv.err(
        "number out of range for conversion to unsigned 64 bit "
        "integer: {}",
        itype );
  return static_cast<uint64_t>( itype );
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
