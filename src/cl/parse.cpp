/****************************************************************
**parse.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Parser for config files.
*
*****************************************************************/
#include "parse.hpp"

// parz
#include "cl/ext-parse.hpp"
#include "parz/runner.hpp"

// base
#include "base/error.hpp"

using namespace std;

namespace cl {

doc parse_file( string_view filename ) {
  UNWRAP_CHECK( d, parz::parse_from_file<cl::doc>( filename ) );
  return std::move( d );
}

} // namespace cl
