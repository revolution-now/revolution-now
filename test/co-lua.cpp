/****************************************************************
**co-lua.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-16.
*
* Description: Unit tests for the src/co-lua.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/co-lua.hpp"

// Revolution Now
#include "src/co-runner.hpp"
#include "src/lua.hpp"
#include "src/luapp/state.hpp"
#include "src/waitable-coro.hpp"

// Must be last.
#include "test/catch-common.hpp"

// C++ standard library
#include <cctype>

FMT_TO_CATCH( ::lua::type );

namespace rn {
namespace {

using namespace std;

using ::Catch::Equals;
using ::Catch::Matches;

string to_lower_str( string_view c ) {
  CHECK( c.size() == 1 );
  char   cl = (char)std::tolower( c[0] );
  string res;
  res += cl;
  return res;
}

#define TRACE( letter ) \
  trace( #letter );     \
  SCOPE_EXIT( trace( to_lower_str( #letter ) ) )

/****************************************************************
** Scenario 1
*****************************************************************/
namespace scenario_1 {

waitable_promise<int> p1;
waitable_promise<int> p2;
waitable_promise<>    p3;
string                shown_int;
string                trace_log;

void trace( string_view msg ) { trace_log += string( msg ); }

waitable<int> do_lua_coroutine() {
  TRACE( A );
  lua::state& st = lua_global_state();
  int         r =
      co_await lua_waitable<int>{}( st["get_and_add_ints"], 5 );
  TRACE( B );
  co_return r;
}

constexpr string_view lua_1 = R"(
  local await = waitable.await

  function TRACE( letter )
    trace( letter )
    return setmetatable( {}, {
      __close = function() trace( string.lower( letter ) ) end
    } )
  end

  function get_and_add_ints( z )
    local _<close> = TRACE( "C" )
    local n = await( get_int_from_user1() )
    local _<close> = TRACE( "D" )
    local m = await( get_int_from_user2() )
    local _<close> = TRACE( "E" )
    await( display_int( n + m + z ) )
    local _<close> = TRACE( "F" )
    return n + m + z
  end

  function throw_error_from_lua( msg )
    local _<close> = TRACE( "P" )
    error( msg )
  end
)";

waitable<int> get_int_from_user1() {
  TRACE( G );
  int result = co_await p1.waitable();
  TRACE( H );
  co_return result;
}

waitable<int> get_int_from_user2() {
  TRACE( I );
  int result = co_await p2.waitable();
  if( result == 42 ) throw runtime_error( "error from cpp" );
  if( result == 43 ) {
    lua::state& st = lua_global_state();
    TRACE( O );
    co_await lua_waitable{}( st["throw_error_from_lua"],
                             "error from lua" );
    SHOULD_NOT_BE_HERE;
  }
  TRACE( J );
  co_return result;
}

waitable<> show_int( int n ) {
  TRACE( K );
  shown_int = to_string( n );
  co_await p3.waitable();
  TRACE( L );
}

waitable<> display_int( int n ) {
  TRACE( M );
  co_await show_int( n ); //
  TRACE( N );
}

void setup( lua::state& st ) {
  st["trace"] = trace;

  st["get_int_from_user1"] = [&]() -> waitable<lua::any> {
    co_return st.cast<lua::any>( co_await get_int_from_user1() );
  };

  st["get_int_from_user2"] = [&]() -> waitable<lua::any> {
    co_return st.cast<lua::any>( co_await get_int_from_user2() );
  };

  st["display_int"] = [&]( int n ) -> waitable<lua::any> {
    co_await display_int( n );
    co_return st.cast<lua::any>( lua::nil );
  };

  st.script.run( lua_1 );
}

} // namespace scenario_1

TEST_CASE( "[co-lua] scenario 1 oneshot" ) {
  using namespace scenario_1;
  lua::state& st = lua_global_state();

  p1        = {};
  p2        = {};
  p3        = {};
  shown_int = {};
  trace_log = {};

  setup( st );

  REQUIRE( !p1.has_value() );
  REQUIRE( !p2.has_value() );
  REQUIRE( !p3.has_value() );
  REQUIRE( shown_int == "" );
  REQUIRE( trace_log == "" );

  waitable<int> w = do_lua_coroutine();
  p1.set_value( 7 );
  p2.set_value( 8 );
  p3.finish();
  run_all_coroutines();
  REQUIRE( trace_log == "ACGHhgDIJjiEMKLlkNnmFfedcBba" );
  REQUIRE( shown_int == "20" );

  REQUIRE( w.ready() );
  REQUIRE( *w == 20 );
}

TEST_CASE( "[co-lua] scenario 1 gradual" ) {
  using namespace scenario_1;
  lua::state& st = lua_global_state();

  p1        = {};
  p2        = {};
  p3        = {};
  shown_int = {};
  trace_log = {};

  setup( st );

  REQUIRE( !p1.has_value() );
  REQUIRE( !p2.has_value() );
  REQUIRE( !p3.has_value() );
  REQUIRE( shown_int == "" );
  REQUIRE( trace_log == "" );

  waitable<int> w = do_lua_coroutine();
  REQUIRE( trace_log == "ACG" );
  run_all_coroutines();
  REQUIRE( trace_log == "ACG" );

  p1.set_value( 7 );
  REQUIRE( trace_log == "ACG" );
  run_all_coroutines();
  REQUIRE( trace_log == "ACGHhgDI" );

  p2.set_value( 8 );
  REQUIRE( trace_log == "ACGHhgDI" );
  REQUIRE( shown_int == "" );
  run_all_coroutines();
  REQUIRE( trace_log == "ACGHhgDIJjiEMK" );
  REQUIRE( shown_int == "20" );

  p3.finish();
  REQUIRE( trace_log == "ACGHhgDIJjiEMK" );
  run_all_coroutines();
  REQUIRE( trace_log == "ACGHhgDIJjiEMKLlkNnmFfedcBba" );

  run_all_coroutines();
  REQUIRE( trace_log == "ACGHhgDIJjiEMKLlkNnmFfedcBba" );
  REQUIRE( shown_int == "20" );

  REQUIRE( w.ready() );
  REQUIRE( *w == 20 );
}

TEST_CASE( "[co-lua] scenario 1 error from cpp" ) {
  using namespace scenario_1;
  lua::state& st = lua_global_state();

  p1        = {};
  p2        = {};
  p3        = {};
  shown_int = {};
  trace_log = {};

  setup( st );

  REQUIRE( !p1.has_value() );
  REQUIRE( !p2.has_value() );
  REQUIRE( !p3.has_value() );
  REQUIRE( shown_int == "" );
  REQUIRE( trace_log == "" );

  waitable<int> w = do_lua_coroutine();
  p1.set_value( 7 );
  p2.set_value( 42 );
  run_all_coroutines();
  REQUIRE( trace_log == "ACGHhgDIidca" );
  REQUIRE( shown_int == "" );

  REQUIRE( !w.ready() );
  REQUIRE( w.has_exception() );
  string msg;
  try {
    std::rethrow_exception( w.exception() );
    REQUIRE( false );
  } catch( runtime_error const& e ) { msg = e.what(); }
  REQUIRE_THAT( msg,
                Matches( ".*test/co-lua.cpp:[0-9]+: \\[string "
                         "\"...\"\\]:15: error from cpp" ) );
}

TEST_CASE( "[co-lua] scenario 1 error from lua" ) {
  using namespace scenario_1;
  lua::state& st = lua_global_state();

  p1        = {};
  p2        = {};
  p3        = {};
  shown_int = {};
  trace_log = {};

  setup( st );

  REQUIRE( !p1.has_value() );
  REQUIRE( !p2.has_value() );
  REQUIRE( !p3.has_value() );
  REQUIRE( shown_int == "" );
  REQUIRE( trace_log == "" );

  waitable<int> w = do_lua_coroutine();
  p1.set_value( 7 );
  p2.set_value( 43 );
  run_all_coroutines();
  REQUIRE( trace_log == "ACGHhgDIOPpoidca" );
  REQUIRE( shown_int == "" );

  REQUIRE( !w.ready() );
  REQUIRE( w.has_exception() );
  string msg;
  try {
    std::rethrow_exception( w.exception() );
    REQUIRE( false );
  } catch( runtime_error const& e ) { msg = e.what(); }
  REQUIRE_THAT(
      msg,
      Matches(
          ".*test/co-lua.cpp:[0-9]+: \\[string "
          "\"...\"\\]:15: .*test/co-lua.cpp:[0-9]+: \\[string "
          "\"...\"\\]:24: error from lua" ) );
}

/****************************************************************
** Scenario 2
*****************************************************************/
namespace scenario_2 {

waitable_promise<int>    p1;
waitable_promise<string> p2;
string                   trace_log;

void trace( string_view msg ) { trace_log += string( msg ); }

waitable<string> accum_cpp( int n ) {
  lua::state& st = lua_global_state();
  TRACE( A );
  if( n == 1 ) {
    TRACE( L );
    string s = co_await p2.waitable();
    TRACE( M );
    if( s == "failed" ) throw runtime_error( "c++ failed" );
    co_return s;
  }
  if( n % 3 == 0 ) {
    TRACE( B );
    int m = n + co_await lua_waitable<int>{}( st["accum_lua"],
                                              n - 1 );
    TRACE( C );
    co_return to_string( m );
  }
  TRACE( D );
  string s = co_await accum_cpp( n - 1 );
  TRACE( E );
  int s_int = stoi( s.c_str() );
  co_return to_string( n + s_int );
}

constexpr string_view lua_1 = R"(
  local await = waitable.await

  function TRACE( letter )
    trace( letter )
    return setmetatable( {}, {
      __close = function() trace( string.lower( letter ) ) end
    } )
  end

  function accum_lua( n )
    local _<close> = TRACE( "F" )
    if n == 1 then
      error( 'should not be here' )
    end
    local _<close> = TRACE( "G" )
    local w = accum_cpp( n-1 )
    local _<close> = TRACE( "H" )
    local r = await( w )
    local _<close> = TRACE( "I" )
    return r
  end
)";

void setup( lua::state& st ) {
  st["trace"]     = trace;
  st["accum_cpp"] = []( int n ) -> waitable<lua::any> {
    lua::state& st = lua_global_state();
    co_return st.cast<lua::any>( co_await accum_cpp( n ) );
  };
  st.script.run( lua_1 );
}

} // namespace scenario_2

TEST_CASE( "[co-lua] scenario 2 oneshot" ) {
  using namespace scenario_2;
  lua::state& st = lua_global_state();

  p1        = {};
  p2        = {};
  trace_log = {};

  setup( st );

  REQUIRE( !p1.has_value() );
  REQUIRE( !p2.has_value() );
  REQUIRE( trace_log == "" );

  waitable<string> w =
      lua_waitable<string>{}( st["accum_lua"], 15 );
  REQUIRE( !w.ready() );
  REQUIRE( trace_log == "FGADADABFGADABFGADABFGADABFGALHHHHH" );

  run_all_coroutines();
  REQUIRE( !w.ready() );
  REQUIRE( trace_log == "FGADADABFGADABFGADABFGADABFGALHHHHH" );

  p2.set_value_emplace( "1" );
  REQUIRE( !w.ready() );
  REQUIRE( trace_log == "FGADADABFGADABFGADABFGADABFGALHHHHH" );

  run_all_coroutines();
  REQUIRE( w.ready() );
  REQUIRE( trace_log ==
           "FGADADABFGADABFGADABFGADABFGALHHHHHMmlaIihgfCcbaEeda"
           "IihgfCcbaEedaIihgfCcbaEedaIihgfCcbaEedaEedaIihgf" );

  REQUIRE( *w == "79" );
}

TEST_CASE( "[co-lua] scenario 2 error" ) {
  using namespace scenario_2;
  lua::state& st = lua_global_state();

  p1        = {};
  p2        = {};
  trace_log = {};

  setup( st );

  REQUIRE( !p1.has_value() );
  REQUIRE( !p2.has_value() );
  REQUIRE( trace_log == "" );

  waitable<string> w =
      lua_waitable<string>{}( st["accum_lua"], 15 );
  REQUIRE( !w.ready() );
  REQUIRE( trace_log == "FGADADABFGADABFGADABFGADABFGALHHHHH" );

  run_all_coroutines();
  REQUIRE( !w.ready() );
  REQUIRE( trace_log == "FGADADABFGADABFGADABFGADABFGALHHHHH" );

  p2.set_value_emplace( "failed" );
  REQUIRE( !w.ready() );
  REQUIRE( trace_log == "FGADADABFGADABFGADABFGADABFGALHHHHH" );

  run_all_coroutines();
  REQUIRE( !w.ready() );
  REQUIRE( trace_log ==
           "FGADADABFGADABFGADABFGADABFGALHHHHHMmlahgfbadahgfbad"
           "ahgfbadahgfbadadahgf" );

  REQUIRE( w.has_exception() );
  string msg;
  try {
    std::rethrow_exception( w.exception() );
    REQUIRE( false );
  } catch( runtime_error const& e ) { msg = e.what(); }
  // clang-format off
  REQUIRE_THAT( msg, Matches(
    ".*test/co-lua.cpp:[0-9]+: "
    "\\[string \"...\"\\]:19: "
    ".*test/co-lua.cpp:[0-9]+: "
    "\\[string \"...\"\\]:19: "
    ".*test/co-lua.cpp:[0-9]+: "
    "\\[string \"...\"\\]:19: "
    ".*test/co-lua.cpp:[0-9]+: "
    "\\[string \"...\"\\]:19: "
    ".*test/co-lua.cpp:[0-9]+: "
    "\\[string \"...\"\\]:19: "
    "c\\+\\+ failed"
  ) );
  // clang-format on
}

} // namespace
} // namespace rn
