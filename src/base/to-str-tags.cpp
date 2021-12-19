/****************************************************************
**to-str-tags.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-19.
*
* Description: Light wrappers for altering to_str behavior.
*
*****************************************************************/
#include "to-str-tags.hpp"

// base-util
#include "base-util/string.hpp"

// Abseil
#include "absl/strings/str_join.h"

using namespace std;

namespace base {
namespace detail {

/****************************************************************
** FmtJsonStyleList
*****************************************************************/
void json_style_list_impl( vector<string> const& items,
                           string&               out ) {
  out += '[';
  out += absl::StrJoin( items, "," );
  out += ']';
}

/****************************************************************
** FmtVerticalJsonList
*****************************************************************/
void vertical_json_list_impl( vector<string> const& items,
                              string&               out ) {
  out += '[';
  if( items.empty() ) {
    out += ']';
    return;
  }
  out += '\n';
  for( int i = 0; i < int( items.size() ); ++i ) {
    string formatted = fmt::to_string( items[i] );
    auto   lines     = util::split( formatted, '\n' );
    for( string_view line : lines )
      out += fmt::format( "  {}\n", line );
    if( i != int( items.size() ) - 1 ) {
      out.resize( out.size() - 1 ); // remove newline.
      out += ',';
    }
    out += '\n';
  }
  out.resize( out.size() - 1 ); // remove newline.
  out += "]";
}

/****************************************************************
** FmtVerticalMap
*****************************************************************/
void vertical_map_impl(
    vector<pair<string, string>> const& items, string& out ) {
  out += '{';
  if( items.empty() ) {
    out += '}';
    return;
  }
  out += '\n';
  int i = 0;
  for( auto const& [k, v] : items ) {
    std::string formatted_k = fmt::to_string( k );
    out += "  ";
    out += formatted_k;
    out += '=';
    std::string formatted_v = fmt::to_string( v );
    auto        lines       = util::split( formatted_v, '\n' );
    for( int j = 0; j < int( lines.size() ); ++j ) {
      if( j != 0 ) out += "  ";
      out += lines[j];
      out += '\n';
    }
    if( i != int( items.size() ) - 1 ) {
      out.resize( out.size() - 1 ); // remove newline.
      out += ',';
      out += '\n';
    }
    ++i;
  }
  out += "}";
}

} // namespace detail
} // namespace base
