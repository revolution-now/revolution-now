/****************************************************************
**lua.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-13.
*
* Description: Lua state initialization.
*
*****************************************************************/
#include "lua.hpp"

// Revolution Now
#include "error.hpp"
#include "expect.hpp"
#include "logger.hpp"

// luapp
#include "luapp/c-api.hpp"
#include "luapp/iter.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/string.hpp"
#include "base/to-str-ext-std.hpp"

// base-util
#include "base-util/io.hpp"
#include "base-util/string.hpp"

using namespace std;

namespace rn {

namespace {

fs::path module_name_to_file_name( string const& m ) {
  string const with_slashes =
      base::str_replace_all( m, { { ".", "/" } } );
  fs::path const file_name = "src/lua/" + with_slashes + ".lua";
  return file_name;
}

lua::table require( lua::state& st, string const& required ) {
  string const key = base::str_replace_all(
      required, { { "-", "_" }, { "/", "." } } );
  lg.debug( "requiring lua module \"{}\".", key );
  lua::table modules =
      lua::table::create_or_get( st["__modules"] );
  if( modules[key] != lua::nil ) {
    LUA_CHECK( st, modules[key] != "loading",
               "cyclic dependency detected." )
    return modules[key].as<lua::table>();
  }
  // The module has not already been loaded.
  lg.info( "loading lua module \"{}\".", key );
  fs::path const file_name =
      module_name_to_file_name( required );
  LUA_CHECK( st, fs::exists( file_name ),
             "file {} does not exist.", file_name );
  // Set the module to something while we're loading in order to
  // detect and break cyclic dependencies.
  modules[key] = "loading";
  lua::table const module_table =
      st.script.run_file<lua::table>( file_name.string() );
  modules[key] = module_table;
  // Create nested tables to hold the module.
  vector<string> const components = base::str_split( key, '.' );
  CHECK_GE( int( components.size() ), 1 );
  lua::table curr = st.table.global();
  for( string const& component : components )
    curr = lua::table::create_or_get( curr[component] );
  // If the table already exists then merge int the new symbols.
  for( auto [k, v] : module_table ) curr[k] = v;
  return module_table;
}

void add_some_members( lua::state& st ) {
  // FIXME
  st.lib.open_all();
  CHECK( st["log"] == lua::nil );
  lua::table log = lua::table::create_or_get( st["log"] );
  log["info"]    = []( string const& msg ) {
    lg.info( "{}", msg );
  };
  log["debug"] = []( string const& msg ) {
    lg.debug( "{}", msg );
  };
  log["trace"] = []( string const& msg ) {
    lg.trace( "{}", msg );
  };
  log["warn"] = []( string const& msg ) {
    lg.warn( "{}", msg );
  };
  log["error"] = []( string const& msg ) {
    lg.error( "{}", msg );
  };
  log["critical"] = []( string const& msg ) {
    lg.critical( "{}", msg );
  };
  // FIXME: needs to be able to take multiple arguments.
  st["print"] = []( lua::any o ) {
    lua::push( o.this_cthread(), o );
    lua::c_api C( o.this_cthread() );
    lg.info( "{}", C.pop_tostring() );
  };
  st["require"] = [&]( string const& name ) {
    return require( st, name );
  };
}

void load_lua_modules( lua::state& st ) {
  for( auto const& path : util::wildcard( "src/lua/*.lua" ) )
    // FIXME FIXME
    // Need to implement the lua methods in the lua-ui module.
    if( !base::str_contains( path.string(), "test.lua" ) )
      require( st, path.stem() );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void run_lua_startup_routines( lua::state& st ) {
  lg.info( "registering Lua functions." );
  for( auto fn : lua::registration_functions() ) ( *fn )( st );
}

void freeze_globals( lua::state& st ) {
  // Freeze all existing global variables and global tables.
  st["meta"]["freeze_all"]();
}

void lua_init( lua::state& st ) {
  add_some_members( st );
  run_lua_startup_routines( st );
  load_lua_modules( st );
}

} // namespace rn
