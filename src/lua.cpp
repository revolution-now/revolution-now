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
#include "base/to-str-ext-std.hpp"

// base-util
#include "base-util/io.hpp"
#include "base-util/string.hpp"

// Abseil
#include "absl/strings/match.h"
#include "absl/strings/str_replace.h"

using namespace std;

namespace rn {

namespace {

lua::table require( lua::state&   st,
                    string const& unsanitized_name ) {
  lua::table ext = lua::table::create_or_get( st["ext"] );
  string     name =
      absl::StrReplaceAll( unsanitized_name, { { "-", "_" } } );
  lg.debug( "requiring lua module \"{}\".", name );
  if( ext[name] != lua::nil ) {
    LUA_CHECK( st, ext[name] != "loading",
               "cyclic dependency detected." )
    return ext[name].as<lua::table>();
  }
  lg.info( "loading lua module \"{}\".", name );
  // Set the module to something while we're loading in order to
  // detect and break cyclic dependencies.
  ext[name] = "loading";
  string with_slashes =
      absl::StrReplaceAll( unsanitized_name, { { ".", "/" } } );
  fs::path file_name = "src/lua/" + with_slashes + ".lua";
  LUA_CHECK( st, fs::exists( file_name ),
             "file {} does not exist.", file_name );
  lua::table module_table =
      st.script.run_file<lua::table>( file_name.string() );
  ext[name] = module_table;
  // In case the symbol already exists we will assume that it is
  // a table and merge its contents into this one.
  auto old_table = lua::table::create_or_get( st[name] );
  st[name]       = module_table;
  for( auto [k, v] : old_table ) st[name][k] = v;
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
    if( !absl::StrContains( path.string(), "test.lua" ) )
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
