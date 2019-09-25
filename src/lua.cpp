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
                        sol::lib::string, sol::lib::debug );
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

// This is for use in the unit tests.
struct MyType {
  int  x{5};
  char get() { return 'c'; }
  int  add( int a, int b ) { return a + b; }
};

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
    return std::isalnum( c ) || ( c == ':' ) || ( c == '.' ) ||
           ( c == '_' );
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
  Vec<Str> segments =
      absl::StrSplit( to_autocomplete, absl::ByAnyChar( ".:" ) );
  CHECK( segments.size() > 0 );
  lg.trace( "segments.size(): {}", segments.size() );
  // FIXME: after upgrading range-v3, use rv::drop_last here.
  auto initial_segments = segments        //
                          | rv::reverse   //
                          | rv::drop( 1 ) //
                          | rv::reverse;

  auto lua_type_string = []( sol::object   parent,
                             string const& key ) {
    // FIXME: this shouldn't be needed, but it seems that sol2
    // reports userdata members always as type function.
    auto pfr = g_lua["meta"]["type_of_child"]( parent, key );
    DCHECK( pfr.valid() );
    auto maybe_res = pfr.get<sol::optional<string>>();
    CHECK( maybe_res );
    return *maybe_res;
  };

  auto sep_for_parent_child = [&]( sol::object   parent,
                                   string const& key ) {
    if( parent.get_type() == sol::type::userdata &&
        lua_type_string( parent, key ) == "function" ) {
      return ':';
    }
    return '.';
  };

  auto table_for_object = []( sol::object o ) {
    Opt<sol::table> res;
    if( o.get_type() == sol::type::table )
      res = o.as<sol::table>();
    if( o.get_type() == sol::type::userdata )
      res = o.as<sol::userdata>()[sol::metatable_key]
                .get<sol::table>();
    return res;
  };

  auto lifted_pairs_for_table = []( sol::table t ) {
    // FIXME: sol2 should access __pairs.
    return g_lua["meta"]["all_pairs"]( t ).get<sol::table>();
  };

  auto table_count_if = [&]( sol::table t, auto&& func ) {
    int  size   = 0;
    auto lifted = lifted_pairs_for_table( t );
    lifted.for_each( [&]( auto const& p ) {
      if( func( lifted, p.first ) ) ++size;
    } );
    return size;
  };

  auto table_size_non_meta = [&]( sol::table t ) {
    return table_count_if( t, []( sol::object /*parent*/,
                                  sol::object key_obj ) {
      if( !key_obj.is<string>() ) return false;
      return !util::starts_with( key_obj.as<string>(), "__" );
    } );
  };

  auto table_fn_members_non_meta = [&]( sol::table t ) {
    return table_count_if( t, [&]( sol::object parent,
                                   sol::object key_obj ) {
      if( !key_obj.is<string>() ) return false;
      auto key = key_obj.as<string>();
      if( util::starts_with( key, "__" ) ) return false;
      return ( lua_type_string( parent, key ) == "function" );
    } );
  };

  sol::table      curr_table = g_lua.globals();
  sol::object     curr_obj   = g_lua.globals();
  sol::state_view st_view( g_lua );

  for( auto piece : initial_segments ) {
    lg.trace( "piece: {}", piece );
    if( piece.empty() ) return {};
    auto maybe_table = table_for_object( curr_table[piece] );
    // lg.trace( "maybe_table: {}", maybe_table.has_value() );
    if( !maybe_table ) return {};
    auto size = table_size_non_meta( *maybe_table );
    // lg.trace( "table_size_non_meta: {}", size );
    if( size == 0 ) return {};
    curr_obj   = curr_table[piece];
    curr_table = *maybe_table;
  }
  auto last = segments.back();
  lg.trace( "last: {}", last );
  string   initial = initial_segments | rv::join( '.' );
  Vec<Str> res;

  auto add_keys = [&]( sol::object parent, auto kv ) {
    sol::object o = kv.first;
    if( o.is<string>() ) {
      string key = o.as<string>();
      lg.trace( "does {} start with last?", key, last );
      if( util::starts_with( key, "__" ) ) return;
      if( util::starts_with( key, last ) ) {
        // lg.trace( "  key: {}", key );
        if( initial.empty() )
          res.push_back( key );
        else {
          auto last_sep = sep_for_parent_child( parent, key );
          lg.trace( "last_sep ({}): {}", key, last_sep );
          res.push_back( initial + last_sep + key );
        }
        res.back() = prefix + res.back();
        lg.trace( "  push_back: {}", res.back() );
      }
    }
  };
  lifted_pairs_for_table( curr_table ).for_each( [&]( auto p ) {
    add_keys( curr_obj, p );
  } );

  sort( res.begin(), res.end() );
  lg.trace( "sorted; size: {}", res.size() );

  for( auto const& match : res ) {
    auto match_dots = absl::StrReplaceAll( match, {{":", "."}} );
    auto fragment_dots =
        absl::StrReplaceAll( fragment, {{":", "."}} );
    DCHECK( util::starts_with( match_dots, fragment_dots ),
            "`{}` does not start with `{}`", match_dots,
            fragment_dots );
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

  DCHECK( res.size() == 1 );
  if( res[0] == fragment ) {
    lg.trace( "res[0], fragment: {},{}", res[0], fragment );
    sol::object o = curr_table[last];
    DCHECK( o != sol::lua_nil );
    if( o.get_type() == sol::type::table ) {
      ASSIGN_CHECK_OPT( t, table_for_object( o ) );
      auto size = table_size_non_meta( t );
      lg.trace( "table size: {}", size );
      if( size > 0 ) res[0] += '.';
    }
    if( o.get_type() == sol::type::userdata ) {
      ASSIGN_CHECK_OPT( t, table_for_object( o ) );
      auto fn_count = table_fn_members_non_meta( t );
      auto size     = table_size_non_meta( t );
      lg.trace( "table size: {}", size );
      lg.trace( "fn count: {}", fn_count );
      if( size > 0 ) {
        if( fn_count == size )
          res[0] += ':';
        else if( fn_count == 0 )
          res[0] += '.';
        else
          res[0] += '.';
      }
    }
    if( lua_type_string( curr_obj, last ) == "function" )
      res[0] += '(';
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
