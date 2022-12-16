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
value to_canonical( converter&, string const& o,
                    tag_t<string> ) {
  return o;
}

result<string> from_canonical( converter& conv, value const& v,
                               tag_t<string> ) {
  UNWRAP_RETURN( str, conv.ensure_type<string>( v ) );
  return str;
}

/****************************************************************
** string_view
*****************************************************************/
value to_canonical( converter&, string_view const& o,
                    tag_t<string_view> ) {
  return string( o );
}

/****************************************************************
** std::filesystem::path
*****************************************************************/
value to_canonical( converter&, fs::path const& o,
                    tag_t<fs::path> ) {
  return o.string();
}

result<fs::path> from_canonical( converter& conv, value const& v,
                                 tag_t<fs::path> ) {
  UNWRAP_RETURN( str, conv.ensure_type<string>( v ) );
  return str;
}

/****************************************************************
** std::chrono::seconds
*****************************************************************/
value to_canonical( converter&, chrono::seconds const& o,
                    tag_t<chrono::seconds> ) {
  return integer_type( o.count() );
}

result<chrono::seconds> from_canonical(
    converter& conv, value const& v, tag_t<chrono::seconds> ) {
  UNWRAP_RETURN( n, conv.ensure_type<integer_type>( v ) );
  return chrono::seconds{ n };
}

/****************************************************************
** std::chrono::milliseconds
*****************************************************************/
value to_canonical( converter&, chrono::milliseconds const& o,
                    tag_t<chrono::milliseconds> ) {
  return integer_type( o.count() );
}

result<chrono::milliseconds> from_canonical(
    converter& conv, value const& v,
    tag_t<chrono::milliseconds> ) {
  UNWRAP_RETURN( n, conv.ensure_type<integer_type>( v ) );
  return chrono::milliseconds{ n };
}

} // namespace cdr
