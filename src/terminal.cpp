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
#include "error.hpp"
#include "logger.hpp"

// luapp
#include "luapp/iter.hpp"
#include "luapp/ruserdata.hpp"
#include "luapp/state.hpp"

// base
#include "base/function-ref.hpp"
#include "base/keyval.hpp"
#include "base/range-lite.hpp"
#include "base/string.hpp"
#include "base/to-str-tags.hpp"

// base-util
#include "base-util/string.hpp"

// C++ standard library
#include <unordered_map>

using namespace std;

namespace rn {

namespace {

namespace rl = ::base::rl;

using ::base::function_ref;
using ::lua::lua_valid;

size_t constexpr max_scrollback_lines = 10000;

unordered_map<string,
              function_ref<void( Terminal& ) const>> const
    kConsoleCommands{
        { "clear",
          []( Terminal& terminal ) { terminal.clear(); } }, //
        { "abort",
          []( Terminal& ) {
            FATAL( "aborting in response to terminal command." );
          } }, //
        { "quit",
          []( Terminal& ) { throw exception_exit{}; } } //
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

vector<string> format_lua_error_msg( string const& msg ) {
  vector<string> res;
  for( auto const& line : util::split_on_any( msg, "\n\r" ) )
    if( !line.empty() ) //
      res.push_back(
          base::str_replace_all( line, { { "\t", "  " } } ) );
  return res;
}

lua_valid run_lua_cmd( Terminal& terminal, lua::state& st,
                       string const& cmd ) {
  lua_valid result = valid;
  // Wrap the command if it's an expression.
  auto cmd_wrapper = cmd;
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
    terminal.log( "lua command failed:" );
    for( auto const& line :
         format_lua_error_msg( result.error() ) )
      terminal.log( "  "s + line );
  }
  return result;
}

lua_valid run_cmd_impl( Terminal& terminal, string const& cmd ) {
  terminal.push_history( cmd );
  terminal.log( "> "s + cmd );
  auto maybe_fn = base::lookup( kConsoleCommands, cmd );
  if( maybe_fn.has_value() ) {
    ( *maybe_fn )( terminal );
    return valid;
  }
  return run_lua_cmd( terminal, terminal.lua_state(), cmd );
}

} // namespace

void Terminal::push_history( std::string const& what ) {
  history_.push_back( what );
}

/****************************************************************
** Terminal Log
*****************************************************************/
// NOTE: this function should only be called while holding the
// g_muffer_mutex.
void Terminal::trim() {
  if( buffer_.size() > max_scrollback_lines )
    buffer_ = vector<string>(
        buffer_.begin() + max_scrollback_lines / 2,
        buffer_.end() );
}

/****************************************************************
** Public API
*****************************************************************/
void Terminal::clear() {
  lock_guard<mutex> lock( buffer_mutex_ );
  buffer_.clear();
}

void Terminal::log( string_view msg ) {
  lock_guard<mutex> lock( buffer_mutex_ );
  buffer_.push_back( string( msg ) );
  trim();
}

lua_valid Terminal::run_cmd( string const& cmd ) {
  if( auto res = run_cmd_impl( *this, cmd ); !res )
    return res.error();
  return valid;
}

maybe<string const&> Terminal::line( int idx ) {
  lock_guard<mutex> lock( buffer_mutex_ );
  if( idx < int( buffer_.size() ) )
    return buffer_[buffer_.size() - 1 - idx];
  return nothing;
}

maybe<string const&> Terminal::history( int idx ) {
  if( idx < int( history_.size() ) )
    return history_[history_.size() - 1 - idx];
  return nothing;
}

vector<string> Terminal::autocomplete( string_view fragment ) {
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
      base::str_split_on_any( to_autocomplete, ".:" );
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
      res = lua::as<lua::table>( o );
    if( lua::type_of( o ) == lua::type::userdata ) {
      lua::userdata ud = lua::as<lua::userdata>( o );

      res = ud[lua::metatable_key]["member_types"]
                .as<lua::table>();
    }
    return res;
  };

  auto lifted_pairs_for_table = [this]( lua::table t ) {
    // FIXME: luapp should have a way to iterate something using
    // proper lua style iteration (which we could then use on the
    // result of the pairs method and would not then need the
    // all_pairs method here). Currently luapp can only iterate
    // through the keys/values in a table that are physically
    // present, as opposed to those produced by a __pairs
    // metamethod.
    return lua::as<lua::table>( st_["meta"]["all_pairs"]( t ) );
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
          return !util::starts_with( lua::as<string>( key_obj ),
                                     "__" );
        } );
  };

  auto table_fn_members_non_meta = [&]( lua::table t ) {
    return table_count_if(
        t, [&]( lua::any parent, lua::any key_obj ) {
          if( lua::type_of( key_obj ) != lua::type::string )
            return false;
          auto key = lua::as<string>( key_obj );
          if( util::starts_with( key, "__" ) ) return false;
          return ( lua::type_of( parent[key] ) ==
                   lua::type::function );
        } );
  };

  lua::table curr_table = st_.table.global();
  lua::any   curr_obj   = lua::any( st_.table.global() );

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
  string initial = base::str_join( initial_segments, "." );
  vector<string> res;

  auto add_keys = [&]( lua::any parent, auto kv ) {
    lua::any o = kv.first;
    if( lua::type_of( o ) == lua::type::string ) {
      string key = lua::as<string>( o );
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
        base::str_replace_all( match, { { ":", "." } } );
    auto fragment_dots =
        base::str_replace_all( fragment, { { ":", "." } } );
    DCHECK( util::starts_with( match_dots, fragment_dots ),
            "`{}` does not start with `{}`", match_dots,
            fragment_dots );
  }

  if( res.size() == 0 ) {
    lg.trace( "returning: {}", base::FmtJsonStyleList{ res } );
    return res;
  } else if( res.size() > 1 ) {
    // Try to find a common prefix.
    auto prefix = util::common_prefix( res );
    CHECK( prefix.has_value() );
    if( prefix->size() > fragment.size() ) res = { *prefix };
    lg.trace( "returning: {}", base::FmtJsonStyleList{ res } );
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
  lg.trace( "returning: {}", base::FmtJsonStyleList{ res } );
  return res;
}

vector<string> Terminal::autocomplete_iterative(
    string_view fragment ) {
  vector<string> res;
  string         single_result( fragment );
  do {
    lg.trace( "single_result: {}", single_result );
    auto try_res = autocomplete( single_result );
    if( try_res.empty() ) break;
    res = std::move( try_res );
    lg.trace( "  res: {}", base::FmtJsonStyleList{ res } );
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

Terminal::Terminal( lua::state& st ) : st_( st ) {}

Terminal::~Terminal() {}

} // namespace rn
