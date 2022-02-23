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

namespace base {

void to_str( string_view o, string& out, ADL_t ) {
  out += string( o );
}

void to_str( string const& o, string& out, ADL_t ) { out += o; }

void to_str( fs::path const& o, string& out, ADL_t ) {
  out += o.string();
};

void to_str( monostate const&, string& out, ADL_t ) {
  out += "monostate";
};

} // namespace base
