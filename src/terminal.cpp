/****************************************************************
**terminal.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-09-25.
*
* Description: Backend for lua terminal.
*
*****************************************************************/
// We  need to define this first because a preprocessor symbol
// de- fined by the {fmt}  library  ("fmt")  conflicts with
// something inside str_join.
#include "absl/strings/str_join.h"

#include "terminal.hpp"

// Revolution Now
#include "error.hpp"
#include "fmt-helper.hpp"
#include "logging.hpp"
#include "lua.hpp"

// luapp
#include "luapp/any.hpp"
#include "luapp/cast.hpp"
#include "luapp/iter.hpp"
#include "luapp/metatable.hpp"
#include "luapp/rstring.hpp"
#include "luapp/ruserdata.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

// base
#include "base/function-ref.hpp"
#include "base/keyval.hpp"
#include "base/range-lite.hpp"

// base-util
#include "base-util/string.hpp"

// Abseil
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"

using namespace std;

namespace rn::term {

namespace rl = ::base::rl;

namespace {

using ::lua::lua_valid;

/****************************************************************
** Global State
*****************************************************************/
size_t constexpr max_scrollback_lines = 10000;
vector<string> g_buffer;
vector<string> g_history;

/****************************************************************
** Terminal Log
*****************************************************************/
void trim() {
  if( g_buffer.size() > max_scrollback_lines )
    g_buffer = vector<string>(
        g_buffer.begin() + max_scrollback_lines / 2,
        g_buffer.end() );
}

/****************************************************************
** Running Commands
*****************************************************************/
unordered_map<string, function_ref<void()>> g_console_commands{
    { "clear", clear }, //
    { "abort",
      [] {
        FATAL( "aborting in response to terminal command." );
      } },                                     //
    { "quit", [] { throw exception_exit{}; } } //
};

bool is_statement( string const& cmd ) {
  return util::contains( cmd, "=" ) ||
         util::contains( cmd, ";" ) ||
         util::starts_with( cmd, "function " );
}

bool is_placeholder( string const& cmd ) {
  if( cmd == "_" ) return true;
  return cmd.size() == 2 && cmd[0] == '_' && cmd[1] >= '0' &&
         cmd[1] <= '9';
}

lua_valid run_lua_cmd( string const& cmd ) {
  lua_valid result = valid;
  // Wrap the command if it's an expression.
  auto        cmd_wrapper = cmd;
  lua::state& st          = lua_global_state();
  if( !is_statement( cmd ) ) {
    lua::any val = st["_"];
    // Wrap command.
    if( !is_placeholder( cmd ) )
      cmd_wrapper = fmt::format(
          "_ = util.print_passthrough(({}))", cmd_wrapper );
    else
      cmd_wrapper = fmt::format( "util.print_passthrough(({}))",
                                 cmd_wrapper );
    if( auto run_result = st.script.run_safe( cmd_wrapper );
        !run_result )
      result = run_result.error();
    if( !is_placeholder( cmd ) && result ) {
      st["_5"] = st["_4"];
      st["_4"] = st["_3"];
      st["_3"] = st["_2"];
      st["_2"] = val;
      // alias.
      st["_1"] = st["_"];
    }
  } else {
    if( auto run_result = st.script.run_safe( cmd_wrapper );
        !run_result )
      result = run_result.error();
  }
  if( !result ) {
    log( "lua command failed:" );
    for( auto const& line :
         format_lua_error_msg( result.error() ) )
      log( "  "s + line );
  }
  return result;
}

lua_valid run_cmd_impl( string const& cmd ) {
  g_history.push_back( cmd );
  log( "> "s + cmd );
  auto maybe_fn = base::lookup( g_console_commands, cmd );
  if( maybe_fn.has_value() ) {
    ( *maybe_fn )();
    return valid;
  }
  return run_lua_cmd( cmd );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void clear() { g_buffer.clear(); }

void log( std::string const& msg ) {
  g_buffer.push_back( msg );
  trim();
}

void log( std::string&& msg ) {
  g_buffer.emplace_back( msg );
  trim();
}

lua_valid run_cmd( string const& cmd ) {
  if( auto res = run_cmd_impl( cmd ); !res ) return res.error();
  return valid;
}

maybe<string const&> line( int idx ) {
  if( idx < int( g_buffer.size() ) )
    return g_buffer[g_buffer.size() - 1 - idx];
  return nothing;
}

maybe<string const&> history( int idx ) {
  if( idx < int( g_history.size() ) )
    return g_history[g_history.size() - 1 - idx];
  return nothing;
}

vector<string> autocomplete( string_view fragment ) {
  auto is_autocomplete_char = []( char c ) {
    return std::isalnum( c ) || ( c == ':' ) || ( c == '.' ) ||
           ( c == '_' );
  };
  lg.trace( "fragment: {}", fragment );
  auto to_autocomplete = rl::rall( fragment )
                             .take_while( is_autocomplete_char )
                             .to_string();
  reverse( to_autocomplete.begin(), to_autocomplete.end() );
  lg.trace( "to_autocomplete: {}", to_autocomplete );
  auto prefix = rl::rall( fragment )
                    .drop( to_autocomplete.size() )
                    .to_string();
  reverse( prefix.begin(), prefix.end() );
  lg.trace( "prefix: {}", prefix );
  // Stop here otherwise we'll be looking through all globals.
  if( to_autocomplete.empty() ) return {};
  // range-v3 split doesn't do the right thing here if the string
  // ends in a dot.
  vector<string> segments =
      absl::StrSplit( to_autocomplete, absl::ByAnyChar( ".:" ) );
  CHECK( segments.size() > 0 );
  lg.trace( "segments.size(): {}", segments.size() );
  // This is a "drop last".
  vector<string> initial_segments(
      segments.begin(),
      segments.empty() ? segments.end() : segments.end() - 1 );

  auto sep_for_parent_child = [&]( lua::any      parent,
                                   string const& key ) {
    if( lua::type_of( parent ) == lua::type::userdata &&
        lua::type_of( parent[key] ) == lua::type::function ) {
      return ':';
    }
    return '.';
  };

  auto table_for_object = []( lua::any o ) {
    maybe<lua::table> res;
    if( lua::type_of( o ) == lua::type::table )
      res = lua::cast<lua::table>( o );
    if( lua::type_of( o ) == lua::type::userdata ) {
      lua::userdata ud = lua::cast<lua::userdata>( o );

      res = ud[lua::metatable_key]["member_types"]
                .cast<lua::table>();
    }
    return res;
  };

  auto lifted_pairs_for_table = []( lua::table t ) {
    // FIXME: luapp should access __pairs.
    return lua::cast<lua::table>(
        lua_global_state()["meta"]["all_pairs"]( t ) );
  };

  auto table_count_if = [&]( lua::table t, auto&& func ) {
    int  size   = 0;
    auto lifted = lifted_pairs_for_table( t );
    for( auto p : lifted )
      if( func( lua::any( lifted ), p.first ) ) ++size;
    return size;
  };

  auto table_size_non_meta = [&]( lua::table t ) {
    return table_count_if(
        t, []( lua::any /*parent*/, lua::any key_obj ) {
          if( lua::type_of( key_obj ) != lua::type::string )
            return false;
          return !util::starts_with(
              lua::cast<string>( key_obj ), "__" );
        } );
  };

  auto table_fn_members_non_meta = [&]( lua::table t ) {
    return table_count_if(
        t, [&]( lua::any parent, lua::any key_obj ) {
          if( lua::type_of( key_obj ) != lua::type::string )
            return false;
          auto key = lua::cast<string>( key_obj );
          if( util::starts_with( key, "__" ) ) return false;
          return ( lua::type_of( parent[key] ) ==
                   lua::type::function );
        } );
  };

  lua::table curr_table = lua_global_state().table.global();
  lua::any   curr_obj =
      lua::any( lua_global_state().table.global() );

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
  string initial = absl::StrJoin( initial_segments, "." );
  vector<string> res;

  auto add_keys = [&]( lua::any parent, auto kv ) {
    lua::any o = kv.first;
    if( lua::type_of( o ) == lua::type::string ) {
      string key = lua::cast<string>( o );
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
  for( auto p : lifted_pairs_for_table( curr_table ) )
    add_keys( curr_obj, p );

  sort( res.begin(), res.end() );
  lg.trace( "sorted; size: {}", res.size() );

  for( auto const& match : res ) {
    auto match_dots =
        absl::StrReplaceAll( match, { { ":", "." } } );
    auto fragment_dots =
        absl::StrReplaceAll( fragment, { { ":", "." } } );
    DCHECK( util::starts_with( match_dots, fragment_dots ),
            "`{}` does not start with `{}`", match_dots,
            fragment_dots );
  }

  if( res.size() == 0 ) {
    lg.trace( "returning: {}", FmtJsonStyleList{ res } );
    return res;
  } else if( res.size() > 1 ) {
    // Try to find a common prefix.
    auto prefix = util::common_prefix( res );
    CHECK( prefix.has_value() );
    if( prefix->size() > fragment.size() ) res = { *prefix };
    lg.trace( "returning: {}", FmtJsonStyleList{ res } );
    return res;
  }

  DCHECK( res.size() == 1 );
  if( res[0] == fragment ) {
    lg.trace( "res[0], fragment: {},{}", res[0], fragment );
    lua::any o = curr_table[last];
    DCHECK( o != lua::nil );
    if( lua::type_of( o ) == lua::type::table ) {
      UNWRAP_CHECK( t, table_for_object( o ) );
      auto size = table_size_non_meta( t );
      lg.trace( "table size: {}", size );
      if( size > 0 ) res[0] += '.';
    }
    if( lua::type_of( o ) == lua::type::userdata ) {
      UNWRAP_CHECK( t, table_for_object( o ) );
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
    if( lua::type_of( curr_obj[last] ) == lua::type::function )
      res[0] += '(';
    lg.trace( "final res[0]: {}", res[0] );
  }
  lg.trace( "returning: {}", FmtJsonStyleList{ res } );
  return res;
}

vector<string> autocomplete_iterative( string_view fragment ) {
  vector<string> res;
  string         single_result( fragment );
  do {
    lg.trace( "single_result: {}", single_result );
    auto try_res = autocomplete( single_result );
    if( try_res.empty() ) break;
    res = std::move( try_res );
    lg.trace( "  res: {}", FmtJsonStyleList{ res } );
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
    if( prefix->size() > fragment.size() ) res = { *prefix };
  }
  return res;
}

} // namespace rn::term

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace rn {
namespace {

LUA_STARTUP( lua::state& st ) {
  // Need to do this somewhere.
  lua::table::create_or_get( st["terminal"] );

  st["log"]["console"] = []( string const& msg ) {
    ::rn::term::log( msg );
  };
};

} // namespace
} // namespace rn
