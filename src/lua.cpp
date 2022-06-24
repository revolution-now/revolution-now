/****************************************************************
**lua.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-13.
*
* Description: Interface to lua.
*
*****************************************************************/
#include "lua.hpp"

// Revolution Now
#include "error.hpp"
#include "expect.hpp"
#include "init.hpp"
#include "logger.hpp"

// game-state
#include "gs/root.hpp"

// luapp
#include "luapp/c-api.hpp"
#include "luapp/ext-userdata.hpp"
#include "luapp/func-push.hpp"
#include "luapp/iter.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"
#include "luapp/usertype.hpp"

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

lua::state g_lua;

lua::table require( string const& unsanitized_name ) {
  lua::state& st  = lua_global_state();
  lua::table  ext = lua::table::create_or_get( st["ext"] );
  string      name =
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
      g_lua.script.run_file<lua::table>( file_name.string() );
  ext[name] = module_table;
  // In case the symbol already exists we will assume that it is
  // a table and merge its contents into this one.
  auto old_table = lua::table::create_or_get( g_lua[name] );
  g_lua[name]    = module_table;
  for( auto [k, v] : old_table ) g_lua[name][k] = v;
  return module_table;
}

void reset_lua_state() {
  g_lua = lua::state{};
  // FIXME
  g_lua.lib.open_all();
  CHECK( g_lua["log"] == lua::nil );
  lua::table log = lua::table::create_or_get( g_lua["log"] );
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
  g_lua["print"] = []( lua::any o ) {
    lua::push( o.this_cthread(), o );
    lua::c_api C( o.this_cthread() );
    lg.info( "{}", C.pop_tostring() );
  };
  g_lua["require"] = require;
}

// This is for use in the unit tests.
struct MyType {
  int    x{ 5 };
  string get() { return "c"; }
  int    add( int a, int b ) { return a + b + x; }
};
NOTHROW_MOVE( MyType );
void to_str( MyType const&, string&, base::ADL_t ) {}

} // namespace
} // namespace rn

namespace lua {
LUA_USERDATA_TRAITS( rn::MyType, owned_by_lua ){};
}

namespace rn {
namespace {

void register_my_type() {
  lua::cthread          L = g_lua.thread.main().cthread();
  lua::usertype<MyType> u( L );

  g_lua["MyType"]        = g_lua.table.create();
  g_lua["MyType"]["new"] = [] { return MyType{}; };

  u["x"]   = &MyType::x;
  u["get"] = &MyType::get;
  u["add"] = &MyType::add;
}

void init_lua() {}

void cleanup_lua() { g_lua = lua::state{}; }

REGISTER_INIT_ROUTINE( lua );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
lua::state& lua_global_state() { return g_lua; }

void run_lua_startup_routines() {
  lg.info( "registering Lua functions." );
  for( auto fn : lua::registration_functions() )
    ( *fn )( g_lua );
  register_my_type(); // for unit testing.
}

void load_lua_modules() {
  for( auto const& path : util::wildcard( "src/lua/*.lua" ) )
    // FIXME FIXME
    // Need to implement the lua methods in the lua-ui module.
    if( !absl::StrContains( path.string(), "test.lua" ) )
      require( path.stem() );
}

void lua_reload( RootState& root_state ) {
  reset_lua_state();
  run_lua_startup_routines();
  load_lua_modules();
  g_lua["ROOT_STATE"] = root_state;
  // Freeze all existing global variables and tables.
  g_lua["meta"]["freeze_all"]();
}

} // namespace rn
