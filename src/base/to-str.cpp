/****************************************************************
**to-str.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-19.
*
* Description: to_str for builtin types.
*
*****************************************************************/
#include "to-str.hpp"

using namespace std;

namespace base {

void to_str( int const& o, string& out, ADL_t ) {
  out += to_string( o );
}

void to_str( uint32_t const& o, string& out, ADL_t ) {
  out += to_string( o );
}

void to_str( long const& o, string& out, ADL_t ) {
  out += to_string( o );
}

void to_str( char o, string& out, ADL_t ) { out += o; }

void to_str( double o, std::string& out, ADL_t ) {
  out += fmt::to_string( o );
}

} // namespace base
