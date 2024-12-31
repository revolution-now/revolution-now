/****************************************************************
**env.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-30.
*
* Description: Environment variables.
*
*****************************************************************/
#include "env.hpp"

// base
#include "conv.hpp"

using namespace std;

namespace base {

maybe<string_view> env_var( char const* const name ) {
  maybe<string_view> res;
  auto const* value = getenv( name );
  if( value != nullptr ) res = value;
  return res;
}

maybe<string_view> user_home_folder() {
  return env_var( "HOME" );
}

maybe<int> os_terminal_columns() {
  return env_var( "COLUMNS" ).bind( []( string_view const sv ) {
    return base::from_chars<int>( sv );
  } );
}

void set_env_var( char const* const var_name,
                  char const* const value ) {
  setenv( var_name, value, /*overwrite=*/true );
}

void set_env_var_if_not_set( char const* const var_name,
                             char const* const value ) {
  setenv( var_name, value, /*overwrite=*/false );
}

void unset_env_var( char const* const name ) {
  unsetenv( name );
}

} // namespace base
