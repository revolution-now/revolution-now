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
  converter conv( "std::string" );
  auto      maybe_str = v.get_if<string>();
  if( !maybe_str.has_value() )
    return conv.err(
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

/****************************************************************
** std::filesystem::path
*****************************************************************/
value to_canonical( fs::path const& o, tag_t<fs::path> ) {
  return o.string();
}

result<fs::path> from_canonical( value const& v,
                                 tag_t<fs::path> ) {
  converter conv( "std::filesystem::path" );
  auto      maybe_str = v.get_if<string>();
  if( !maybe_str.has_value() )
    return conv.err(
        "producing a std::filesystem::path requires type "
        "string, instead found type {}.",
        type_name( v ) );
  return *maybe_str;
}

/****************************************************************
** std::chrono::seconds
*****************************************************************/
value to_canonical( chrono::seconds const& o,
                    tag_t<chrono::seconds> ) {
  return integer_type( o.count() );
}

result<chrono::seconds> from_canonical(
    value const& v, tag_t<chrono::seconds> ) {
  converter conv( "std::chrono::seconds" );
  auto      maybe_int = v.get_if<integer_type>();
  if( !maybe_int.has_value() )
    return conv.err(
        "producing a std::chrono::seconds requires type "
        "integer, instead found type {}.",
        type_name( v ) );
  return chrono::seconds{ *maybe_int };
}

} // namespace cdr
