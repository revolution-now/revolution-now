/****************************************************************
**util.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Common utilities for all modules.
*
*****************************************************************/
#include "util.hpp"

// Revolution Now
#include "errors.hpp"

// C++ standard library.
#include <cstdlib>
#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>

using namespace std;

namespace rn {

namespace {

Opt<string> environment_variable( char const* name ) {
  Opt<string> res;
  auto const* value = getenv( name );
  if( value != nullptr ) res = value;
  return res;
}

} // namespace

int round_up_to_nearest_int_multiple( double d, int m ) {
  if( d < 0.0 )
    return -round_down_to_nearest_int_multiple( -d, m );
  int floor = int( d );
  if( floor % m != 0 ) floor += m;
  return floor / m;
}

int round_down_to_nearest_int_multiple( double d, int m ) {
  if( d < 0.0 )
    return -round_up_to_nearest_int_multiple( -d, m );
  int floor = int( d );
  return floor / m;
}

Opt<fs::path> user_home_folder() {
  return environment_variable( "HOME" );
}

} // namespace rn
