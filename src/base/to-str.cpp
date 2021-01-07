/****************************************************************
**to-str.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-07.
*
* Description: to_str framework.
*
*****************************************************************/
#include "to-str.hpp"

using namespace std;

namespace base {

void to_str( int const& o, std::string& out ) {
  out += std::to_string( o );
}

void to_str( std::string const& o, std::string& out ) {
  out += o;
}

} // namespace base
