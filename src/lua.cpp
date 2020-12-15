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
#include "errors.hpp"
#include "fmt-helper.hpp"
#include "init.hpp"
#include "logging.hpp"

// base-util
#include "base-util/io.hpp"
#include "base-util/string.hpp"

// Abseil
#include "absl/strings/str_replace.h"

using namespace std;

namespace rn::lua {

namespace {

sol::state g_lua;

auto& registration_functions() {
  static vector<pair<string, RegistrationFnSig**>> fns;
  return fns;
}

bool is_valid_lua_identifier( string_view name ) {
  // Good enough for now.
  return !util::contains( name, " " ) &&
         !util::contains( name, "." );
}

expect<> load_module( string const& name ) {
  lg.info( "loading lua module \"{}\".", name );
  CHECK( is_valid_lua_identifier( name ),
         "module name `{}` is not a valid lua identifier.",
         name );
  fs::path file_name = "src/lua/" + name + ".lua";
  CHECK( fs::exists( file_name ), "file {} does not exist.",
         file_name );
  g_lua["package_exports"] = sol::lua_nil;
  auto pf_result           = g_lua.safe_script_file(
      "src/lua/" + name + ".lua",
      []( auto*, auto pfr ) { return pfr; } );
  if( !pf_result.valid() ) {
    sol::error err = pf_result;
    return UNEXPECTED( err.what() );
  }
  CHECK( g_lua["package_exports"] != sol::lua_nil,
         "module `{}` does not have package exports.", name );
  // In case the symbol already exists we will assume that it is
  // a table and merge its contents into this one.
  auto old_table = g_lua[name].get_or_create<sol::table>();
  g_lua[name]    = g_lua["package_exports"];
  for( auto [k, v] : old_table ) g_lua[name][k] = v;

  g_lua["package_exports"] = sol::lua_nil;
  return xp_success_t{};
}

void reset_sol_state() {
  g_lua = sol::state{};
  g_lua.open_libraries( sol::lib::base, sol::lib::table,
                        sol::lib::string, sol::lib::debug );
  CHECK( g_lua["log"] == sol::lua_nil );
  g_lua["log"].get_or_create<sol::table>();
  g_lua["log"]["info"] = []( string const& msg ) {
    lg.info( "{}", msg );
  };
  g_lua["log"]["debug"] = []( string const& msg ) {
    lg.debug( "{}", msg );
  };
  g_lua["log"]["trace"] = []( string const& msg ) {
    lg.trace( "{}", msg );
  };
  g_lua["log"]["warn"] = []( string const& msg ) {
    lg.warn( "{}", msg );
  };
  g_lua["log"]["error"] = []( string const& msg ) {
    lg.error( "{}", msg );
  };
  g_lua["log"]["critical"] = []( string const& msg ) {
    lg.critical( "{}", msg );
  };
  g_lua["print"] = []( sol::object o ) {
    if( o == sol::lua_nil ) return;
    if( auto maybe_string = o.as<maybe<string>>();
        maybe_string.has_value() ) {
      if( *maybe_string == "nil" ) return;
      lg.info( "{}", *maybe_string );
    } else if( auto maybe_bool = o.as<maybe<bool>>();
               maybe_bool.has_value() )
      lg.info( "{}", *maybe_bool );
    else if( auto maybe_double = o.as<maybe<double>>();
             maybe_double.has_value() )
      lg.info( "{}", *maybe_double );
    else
      lg.info( "(print: object cannot be converted to string)" );
  };
}

// This is for use in the unit tests.
struct MyType {
  int  x{ 5 };
  char get() { return 'c'; }
  int  add( int a, int b ) { return a + b; }
};
NOTHROW_MOVE( MyType );

void register_my_type() {
  sol::usertype<MyType> u = g_lua.new_usertype<MyType>(
      "MyType", sol::constructors<MyType()>{} );

  u["x"]   = &MyType::x;
  u["get"] = &MyType::get;
  u["add"] = &MyType::add;
}

void init_lua() {}

void cleanup_lua() { g_lua = sol::state{}; }

REGISTER_INIT_ROUTINE( lua );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
sol::state& global_state() { return g_lua; }

void run_startup_routines() {
  lg.info( "registering Lua functions." );
  auto modules = g_lua["modules"].get_or_create<sol::table>();
  for( auto const& [mod_name, fn] : registration_functions() ) {
    modules[mod_name] = "module";
    ( *fn )( g_lua );
    CHECK( g_lua[mod_name] != sol::lua_nil,
           "module \"{}\" has not been defined.", mod_name );
  }
  register_my_type(); // for unit testing.
}

void load_modules() {
  auto modules = g_lua["modules"].get_or_create<sol::table>();
  for( auto const& path : util::wildcard( "src/lua/*.lua" ) ) {
    string stem = path.stem();
    CHECK_XP( load_module( stem ) );
    modules[stem] = "module";
  }
}

void run_startup_main() {
  CHECK_XP( lua::run<void>( "startup.main()" ) );
}

void reload() {
  reset_sol_state();
  run_startup_routines();
  load_modules();
  // Freeze all existing global variables and tables.
  CHECK_XP( run<void>( "meta.freeze_all()" ) );
}

vector<string> format_lua_error_msg( string const& msg ) {
  vector<string> res;
  for( auto const& line : util::split_on_any( msg, "\n\r" ) )
    if( !line.empty() ) //
      res.push_back(
          absl::StrReplaceAll( line, { { "\t", "  " } } ) );
  return res;
}

void register_fn( string_view         module_name,
                  RegistrationFnSig** fn ) {
  registration_functions().emplace_back( module_name, fn );
}

/****************************************************************
** Testing
*****************************************************************/
void test_lua() {
  sol::state st;
  st.open_libraries( sol::lib::base );
  enum class color { red, blue };
  constexpr bool ro = false;
  st.new_enum<color, /*read_only=*/ro>(
      "color",
      { { "red", color::red }, { "blue", color::blue } } );
  auto script = R"lua(
    print( "start" )
    for k, v in pairs( color ) do
      print( tostring(k) .. ": " .. tostring(v) )
    end
    print( "end" )
  )lua";
  st.safe_script( script );
}

void reset_state() { reload(); }

} // namespace rn::lua
