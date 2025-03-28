/****************************************************************
**to-str-ext-std.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-18.
*
* Description: to_str from std types.
*
*****************************************************************/
#include "to-str-ext-std.hpp"

using namespace std;

namespace std {

void to_str( string_view const& o, string& out,
             base::tag<string_view> ) {
  out += string( o );
}

void to_str( string const& o, string& out, base::tag<string> ) {
  out += o;
}

void to_str( filesystem::path const& o, string& out,
             base::tag<filesystem::path> ) {
  out += o.string();
};

void to_str( monostate const&, string& out,
             base::tag<monostate> ) {
  out += "monostate";
};

void to_str( source_location const& o, string& out,
             base::tag<source_location> ) {
  out += fmt::format( "{}:{}:{}", o.file_name(), o.line(),
                      o.column() );
};

void to_str( nullptr_t const&, std::string& out,
             base::tag<nullptr_t> ) {
  out += "nullptr";
}

} // namespace std
