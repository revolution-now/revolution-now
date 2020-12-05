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
#include "terminal.hpp"

// Revolution Now
#include "errors.hpp"
#include "logging.hpp"
#include "lua.hpp"
#include "ranges.hpp"

// base
#include "base/function-ref.hpp"
#include "base/keyval.hpp"

// Abseil
#include "absl/strings/str_replace.h"
#include "absl/strings/str_split.h"

// Range-v3
#include "range/v3/view/drop.hpp"
#include "range/v3/view/join.hpp"
#include "range/v3/view/reverse.hpp"
#include "range/v3/view/take_while.hpp"

using namespace std;

namespace rn::term {

namespace {

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
FlatMap<string, function_ref<void()>> g_console_commands{
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

expect<> run_lua_cmd( string const& cmd ) {
  expect<> expected;
  // Wrap the command if it's an expression.
  auto cmd_wrapper = cmd;
  if( !is_statement( cmd ) ) {
    sol::state_view st = lua::global_state();
    sol::lua_value  val( st );
    val = st["_"];
    // Wrap command.
    if( !is_placeholder( cmd ) )
      cmd_wrapper = fmt::format(
          "_ = util.print_passthrough(({}))", cmd_wrapper );
    else
      cmd_wrapper = fmt::format( "util.print_passthrough(({}))",
                                 cmd_wrapper );
    expected = lua::run<void>( cmd_wrapper );
    if( !is_placeholder( cmd ) && expected ) {
      st["_5"] = st["_4"];
      st["_4"] = st["_3"];
      st["_3"] = st["_2"];
      st["_2"] = val;
      // alias.
      st["_1"] = st["_"];
    }
  } else {
    expected = lua::run<void>( cmd_wrapper );
  }
  if( !expected.has_value() ) {
    log( "lua command failed:" );
    for( auto const& line :
         lua::format_lua_error_msg( expected.error().what ) )
      log( "  "s + line );
  }
  return expected;
}

expect<> run_cmd_impl( string const& cmd ) {
  g_history.push_back( cmd );
  log( "> "s + cmd );
  auto maybe_fn = base::lookup( g_console_commands, cmd );
  if( maybe_fn.has_value() ) {
    ( *maybe_fn )();
    return xp_success_t{};
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

expect<> run_cmd( string const& cmd ) {
  return run_cmd_impl( cmd );
}

maybe<string const&> line( int idx ) {
  if( idx < int( g_buffer.size() ) )
    return g_buffer[g_buffer.size() - 1 - idx];
  return nothing;
}

maybe<string const&> history( int idx ) {
  if( idx < int( g_history.size() ) )
    g_history[g_history.size() - 1 - idx];
  return nothing;
}

Vec<Str> autocomplete( string_view fragment ) {
  auto is_autocomplete_char = []( char c ) {
    return std::isalnum( c ) || ( c == ':' ) || ( c == '.' ) ||
           ( c == '_' );
  };
  lg.trace( "fragment: {}", fragment );
  auto to_autocomplete =
      rg::to<string>( fragment                                 //
                      | rv::reverse                            //
                      | rv::take_while( is_autocomplete_char ) //
                      | rv::reverse );
  lg.trace( "to_autocomplete: {}", to_autocomplete );
  auto prefix =
      rg::to<string>( fragment                             //
                      | rv::reverse                        //
                      | rv::drop( to_autocomplete.size() ) //
                      | rv::reverse );
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
    auto pfr = lua::global_state()["meta"]["type_of_child"](
        parent, key );
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
    return lua::global_state()["meta"]["all_pairs"]( t )
        .get<sol::table>();
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

  sol::table      curr_table = lua::global_state().globals();
  sol::object     curr_obj   = lua::global_state().globals();
  sol::state_view st_view( lua::global_state() );

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
  auto initial =
      rg::to<string>( initial_segments | rv::join( '.' ) );
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
  lg.trace( "returning: {}", FmtJsonStyleList{ res } );
  return res;
}

Vec<Str> autocomplete_iterative( string_view fragment ) {
  Vec<Str> res;
  string   single_result( fragment );
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

LUA_STARTUP( sol::state& st ) {
  // Need to do this somewhere.
  st["terminal"].get_or_create<sol::table>();

  st["log"]["console"] = []( string const& msg ) {
    ::rn::term::log( msg );
  };
};

} // namespace
} // namespace rn
