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
#include "aliases.hpp"
#include "errors.hpp"
#include "fmt-helper.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "lua-ext.hpp"
#include "ranges.hpp"

// base-util
#include "base-util/io.hpp"
#include "base-util/string.hpp"

// Abseil
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"

// Range-v3
#include "range/v3/view/drop.hpp"
#include "range/v3/view/join.hpp"
#include "range/v3/view/reverse.hpp"
#include "range/v3/view/take_while.hpp"

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

expect<monostate> load_module( string const& name ) {
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
  return monostate{};
}

void reset_sol_state() {
  g_lua = sol::state{};
  g_lua.open_libraries( sol::lib::base, sol::lib::table,
                        sol::lib::string );
  CHECK( g_lua["log"] == sol::lua_nil );
  g_lua["log"].get_or_create<sol::table>();
  g_lua["log"]["info"] = []( string const& msg ) {
    lg.info( "{}", msg );
  };
  g_lua["log"]["debug"] = []( string const& msg ) {
    lg.debug( "{}", msg );
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
    if( auto maybe_string = o.as<Opt<string>>();
        maybe_string.has_value() ) {
      if( *maybe_string == "nil" ) return;
      lg.info( "{}", *maybe_string );
    } else if( auto maybe_bool = o.as<Opt<bool>>();
               maybe_bool.has_value() )
      lg.info( "{}", *maybe_bool );
    else if( auto maybe_double = o.as<Opt<double>>();
             maybe_double.has_value() )
      lg.info( "{}", *maybe_double );
    else
      lg.info( "(print: object cannot be converted to string)" );
  };
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

Vec<Str> format_lua_error_msg( Str const& msg ) {
  Vec<Str> res;
  for( auto const& line : util::split_on_any( msg, "\n\r" ) )
    if( !line.empty() ) //
      res.push_back(
          absl::StrReplaceAll( line, {{"\t", "  "}} ) );
  return res;
}

void register_fn( string_view         module_name,
                  RegistrationFnSig** fn ) {
  registration_functions().emplace_back( module_name, fn );
}

Vec<Str> autocomplete( std::string_view fragment ) {
  auto is_autocomplete_char = []( char c ) {
    return std::isalnum( c ) || ( c == '.' ) || ( c == '_' );
  };
  lg.trace( "fragment: {}", fragment );
  string to_autocomplete =
      fragment                                 //
      | rv::reverse                            //
      | rv::take_while( is_autocomplete_char ) //
      | rv::reverse;
  lg.trace( "to_autocomplete: {}", to_autocomplete );
  string prefix = fragment                             //
                  | rv::reverse                        //
                  | rv::drop( to_autocomplete.size() ) //
                  | rv::reverse;
  lg.trace( "prefix: {}", prefix );
  // Stop here otherwise we'll be looking through all globals.
  if( to_autocomplete.empty() ) return {};
  // range-v3 split doesn't do the right thing here if the string
  // ends in a dot.
  Vec<Str> segments = absl::StrSplit( to_autocomplete, '.' );
  CHECK( segments.size() > 0 );
  lg.trace( "segments.size(): {}", segments.size() );
  // FIXME: after upgrading range-v3, use rv::drop_last here.
  auto initial_segments = segments        //
                          | rv::reverse   //
                          | rv::drop( 1 ) //
                          | rv::reverse;
  sol::table      curr_table = g_lua.globals();
  sol::state_view st_view( g_lua );
  for( auto piece : initial_segments ) {
    lg.trace( "piece: {}", piece );
    if( curr_table[piece] == sol::lua_nil ) return {};
    sol::object o = curr_table[piece];
    DCHECK( o != sol::lua_nil );
    bool is_table_like = ( o.get_type() == sol::type::table );
    lg.trace( "is_table_like: {}", is_table_like );
    if( !is_table_like ) return {};
    curr_table = o.as<sol::table>();
  }
  auto last = segments.back();
  lg.trace( "last: {}", last );
  string   initial = initial_segments | rv::join( '.' );
  Vec<Str> res;
  auto     add_keys = [&]( auto p ) {
    sol::object o = p.first;
    if( o.is<string>() ) {
      string key = o.as<string>();
      if( util::starts_with( key, last ) ) {
        lg.trace( "[k,v]" );
        lg.trace( "  key: {}", key );
        auto match =
            initial.empty() ? key : ( initial + '.' + key );
        res.push_back( prefix + match );
        lg.trace( "  push_back: {}", match );
      }
    }
  };
  // FIXME: sol2 should access __pairs.
  sol::table lifted = g_lua["meta"]["all_pairs"]( curr_table );
  lifted.for_each( add_keys );

  sort( res.begin(), res.end() );
  lg.trace( "sorted; size: {}", res.size() );

  for( auto const& match : res ) {
    DCHECK( util::starts_with( match, fragment ) );
  }

  if( res.size() == 0 ) {
    lg.trace( "returning: {}", FmtJsonStyleList{res} );
    return res;
  } else if( res.size() > 1 ) {
    // Try to find a common prefix.
    auto prefix = util::common_prefix( res );
    CHECK( prefix.has_value() );
    if( prefix->size() > fragment.size() ) res = {*prefix};
    lg.trace( "returning: {}", FmtJsonStyleList{res} );
    return res;
  }

  auto table_size = []( sol::table t ) {
    int size = 0;
    t.for_each( [&]( auto const& ) { ++size; } );
    return size;
  };

  DCHECK( res.size() == 1 );
  if( res[0] == fragment ) {
    lg.trace( "res[0], fragment: {},{}", res[0], fragment );
    sol::object o = curr_table[last];
    DCHECK( o != sol::lua_nil );
    bool is_table_like = ( o.get_type() == sol::type::table );
    lg.trace( "is_table_like: {}", is_table_like );
    if( is_table_like ) {
      auto t = o.as<sol::table>();
      // FIXME: sol2 should access __pairs.
      sol::table lifted = g_lua["meta"]["all_pairs"]( t );
      auto       size   = table_size( lifted );
      lg.trace( "table size: {}", size );
      if( size > 0 ) res[0] += '.';
    }
    if( o.is<sol::function>() ) { res[0] += '('; }
    lg.trace( "final res[0]: {}", res[0] );
  }
  lg.trace( "returning: {}", FmtJsonStyleList{res} );
  return res;
}

Vec<Str> autocomplete_iterative( std::string_view fragment ) {
  Vec<Str> res;
  string   single_result( fragment );
  do {
    lg.trace( "single_result: {}", single_result );
    auto try_res = autocomplete( single_result );
    if( try_res.empty() ) break;
    res = std::move( try_res );
    lg.trace( "  res: {}", FmtJsonStyleList{res} );
    if( res.size() == 1 ) {
      lg.trace( "  size is 1" );
      if( single_result == res[0] ) break;
      lg.trace( "  not equal" );
      single_result = res[0];
    }
  } while( res.size() == 1 );
  if( res.size() > 1 ) {
    // Try to find a common prefix.
    auto prefix = util::common_prefix( res );
    CHECK( prefix.has_value() );
    if( prefix->size() > fragment.size() ) res = {*prefix};
  }
  return res;
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
      "color", {{"red", color::red}, {"blue", color::blue}} );
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
