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

// base-util
#include "base-util/optional.hpp"
#include "base-util/string.hpp"

// C++ standard library.
#include <cstdlib>
#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>

using namespace std;

namespace rn {

Opt<string_view> env_var( char const* name ) {
  Opt<string_view> res;
  auto const*      value = getenv( name );
  if( value != nullptr ) res = value;
  return res;
}

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

Opt<fs::path> user_home_folder() { return env_var( "HOME" ); }

Opt<int> os_terminal_columns() {
  using util::infix::fmap_join;
  return env_var( "COLUMNS" ) |
         fmap_join( L( util::from_chars<int>( _ ) ) );
}

void set_env_var( char const* var_name, char const* value ) {
  setenv( var_name, value, /*overwrite=*/true );
}

void set_env_var_if_not_set( char const* var_name,
                             char const* value ) {
  setenv( var_name, value, /*overwrite=*/false );
}

void unset_env_var( char const* name ) { unsetenv( name ); }

} // namespace rn
