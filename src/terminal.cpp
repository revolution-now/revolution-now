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
#include "interrupts.hpp"

// luapp
#include "luapp/state.hpp"

// base
#include "base/function-ref.hpp"
#include "base/keyval.hpp"
#include "base/logger.hpp"
#include "base/string.hpp"

// base-util
#include "base-util/string.hpp"

// C++ standard library
#include <algorithm>
#include <unordered_map>

using namespace std;

namespace rn {

namespace {

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
        []( Terminal& ) { throw exception_hard_exit{}; } } //
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
         format_lua_error_msg( result.error().msg ) )
      terminal.log( "  "s + line );
  }
  return result;
}

lua_valid run_cmd_impl( Terminal& terminal, lua::state& state,
                        string const& cmd ) {
  terminal.push_history( cmd );
  terminal.log( "> "s + cmd );
  auto maybe_fn = base::lookup( kConsoleCommands, cmd );
  if( maybe_fn.has_value() ) {
    ( *maybe_fn )( terminal );
    return valid;
  }
  return run_lua_cmd( terminal, state, cmd );
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

lua_valid Terminal::run_cmd( lua::state& state,
                             string const& cmd ) {
  if( auto res = run_cmd_impl( *this, state, cmd ); !res )
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

Terminal::Terminal() {}

Terminal::~Terminal() {}

} // namespace rn
