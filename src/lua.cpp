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
#include "config-lua.hpp"
#include "error.hpp"
#include "expect.hpp"
#include "iengine.hpp"
#include "irand.hpp"

// luapp
#include "luapp/c-api.hpp"
#include "luapp/func-push.hpp"
#include "luapp/iter.hpp"
#include "luapp/register.hpp"
#include "luapp/rfunction.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"
#include "base/string.hpp"
#include "base/to-str-ext-std.hpp"

// base-util
#include "base-util/io.hpp"
#include "base-util/string.hpp"

// C++ standard library
#include <bit>

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
  lua::table const dummy  = st.table.create();
  lua::table module_table = dummy;
  if( modules[key] != lua::nil ) {
    LUA_CHECK( st, modules[key] != "loading",
               "cyclic dependency detected." )
    module_table = modules[key].as<lua::table>();
    return module_table;
  }
  // The module has not already been loaded.
  lg.debug( "loading lua module \"{}\".", key );
  fs::path const file_name =
      module_name_to_file_name( required );
  LUA_CHECK( st, fs::exists( file_name ),
             "file {} does not exist.", file_name );
  // Set the module to something while we're loading in order to
  // detect and break cyclic dependencies.
  modules[key] = "loading";
  module_table =
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

void add_logging_methods( lua::state& st ) {
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
}

void load_lua_modules( lua::state& st ) {
  for( auto const& path : util::wildcard( "src/lua/*.lua" ) )
    if( path.string().find( "test" ) == string::npos )
      require( st, path.stem() );
}

void set_up_lua_rng( IEngine& engine, lua::state& st ) {
  CHECK( st["math"]["random"] == lua::nil &&
             st["math"]["randomseed"] == lua::nil,
         "lua rng facilities should be removed from the binary "
         "as lua should generate random numbers through the C++ "
         "API." );

  IRand& rand = engine.rand();

  // This seeks to reproduce Lua 5.4's math.random function.
  lua::rfunction const math_random = st.script.load( R"lua(
    -- NOTE: the math functions used below are injected from C++.
    local function random( l, r )
      if l == nil and r == nil then
        return math.uniform_double( 0.0, 1.0 )
      end
      assert( l >= 0, 'interval is empty' )
      if not r then
        if l == 0 then
          return math.uniform_int64()
        end
        return math.uniform_int( 1, l ) -- starts at 1.
      end
      assert( l <= r, 'interval is empty' )
      return math.uniform_int( l, r )
    end
    return random
  )lua" );

  // Define this as a lua function because currently our lua
  // framework can't expose C++ functions with an arbitrary
  // number of parameters, and we want this to give the desired
  // error no matter how many parameters it is called with.
  lua::rfunction const math_randomseed = st.script.load( R"lua(
    local function dummy()
      error( "math.randomseed should not be used." )
    end
    return dummy
  )lua" );

  auto const uniform_int = [&rand]( int const l, int const r ) {
    return rand.uniform_int( l, r );
  };

  auto const uniform_int64 = [&rand] {
    // A little hacky, but saves us from having to add another
    // api method for this.
    rng::entropy const e = rand.generate_deterministic_seed();
    uint64_t const ui =
        ( uint64_t( e.e2 ) << 32 ) + uint64_t( e.e1 );
    return bit_cast<int64_t>( ui );
  };

  auto const uniform_double = [&rand]( double const l,
                                       double const r ) {
    return rand.uniform_double( l, r );
  };

  // These are used above to implement our math.random, but they
  // can also be called directly.
  st["math"]["uniform_int"]    = auto( uniform_int );
  st["math"]["uniform_int64"]  = auto( uniform_int64 );
  st["math"]["uniform_double"] = auto( uniform_double );

  st["math"]["random"]     = math_random;
  st["math"]["randomseed"] = math_randomseed;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void run_lua_startup_routines( lua::state& st ) {
  lg.info( "registering Lua functions." );
  for( auto const fn : lua::registration_functions() )
    ( *fn )( st );
}

void lua_init( IEngine&, lua::state& st ) {
  st.lib.open_all();

  (void)set_up_lua_rng; //( engine, st );

  st["require"] = [&]( string const& name ) {
    return require( st, name );
  };

  // NOTE: these will not be frozen when injected; it is expected
  // that all globals will be recursively frozen at the end of
  // the init process.
  inject_configs( st );

  run_lua_startup_routines( st );

  add_logging_methods( st );
  load_lua_modules( st );

  // Freeze all existing global variables and global tables.
  st["require"]( "util.freeze" )["freeze_environment"]();
}

} // namespace rn
