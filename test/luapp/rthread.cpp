/****************************************************************
**rthread.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: Unit tests for the src/luapp/thread.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/rthread.hpp"

// Testing
#include "test/luapp/common.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {
namespace {

using namespace std;

using ::base::maybe;
using ::base::valid;

/****************************************************************
** rthread objects
*****************************************************************/
LUA_TEST_CASE( "[rthread] rthread equality" ) {
  boolean       b   = true;
  integer       i   = 1;
  floating      f   = 2.3;
  lightuserdata lud = C.newuserdata( 10 );
  C.pop();

  (void)C.newthread();
  rthread o1( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );

  (void)C.newthread();
  rthread o2( C.this_cthread(), C.ref_registry() );
  REQUIRE( C.stack_size() == 0 );

  rthread o3 = o1;

  REQUIRE( o1 != o2 );
  REQUIRE( o1 == o3 );

  REQUIRE( o1 != nil );
  REQUIRE( o1 != b );
  REQUIRE( o1 != i );
  REQUIRE( o1 != f );
  REQUIRE( o1 != lud );
  REQUIRE( nil != o1 );
  REQUIRE( b != o1 );
  REQUIRE( i != o1 );
  REQUIRE( f != o1 );
  REQUIRE( lud != o1 );
}

LUA_TEST_CASE( "[rthread] construction + is_main" ) {
  C.pushthread();
  rthread th1( L, C.ref_registry() );
  REQUIRE( th1.is_main() );
  C.pushthread();
  rthread th2( L, C.ref_registry() );
  REQUIRE( th2.is_main() );
  REQUIRE( th1 == th2 );

  cthread NL = C.newthread();
  rthread th3( L, C.ref_registry() );
  REQUIRE( !th3.is_main() );
  REQUIRE( th3.cthread() == NL );
  REQUIRE( th3 != th1 );
  REQUIRE( th3 != th2 );

  lua::push( L, th3 );
  maybe<rthread> m = lua::get<rthread>( L, -1 );
  REQUIRE( m.has_value() );
  REQUIRE( m == th3 );
  REQUIRE( m != th1 );
  REQUIRE( !m->is_main() );
  C.pop();
}

LUA_TEST_CASE( "[lua-state] thread resume unsafe" ) {
  C.openlibs();
  st.script.run( R"(
  function f()
    coroutine.yield()
    local n = coroutine.yield( "hello" )
    assert( n == 6 )
    local m = coroutine.yield( n*2 )
    assert( m == 7 )
    return n+m
  end
  )" );
  rfunction f = st["f"].cast<rfunction>();

  rthread coro = st.thread.create_coro( f );
  REQUIRE( coro.status() == thread_status::ok );
  REQUIRE( coro.coro_status() == coroutine_status::suspended );
  coro.resume();
  REQUIRE( coro.status() == thread_status::yield );
  REQUIRE( coro.coro_status() == coroutine_status::suspended );
  string s = coro.resume<string>();
  REQUIRE( coro.status() == thread_status::yield );
  REQUIRE( coro.coro_status() == coroutine_status::suspended );
  REQUIRE( s == "hello" );
  int i = coro.resume<int>( 6 );
  REQUIRE( coro.status() == thread_status::yield );
  REQUIRE( coro.coro_status() == coroutine_status::suspended );
  REQUIRE( i == 12 );
  int res = coro.resume<int>( 7 );
  REQUIRE( coro.status() == thread_status::ok );
  REQUIRE( coro.coro_status() == coroutine_status::dead );
  REQUIRE( res == 13 );
}

LUA_TEST_CASE( "[lua-state] thread resume safe w/ error" ) {
  C.openlibs();
  st.script.run( R"(
  function f()
    coroutine.yield()
    local n = coroutine.yield( "hello" )
    assert( n == 6, 'n is incorrect' )
    error( 'some error' )
  end
  )" );
  rfunction f = st["f"].cast<rfunction>();

  rthread coro = st.thread.create_coro( f );
  REQUIRE( coro.status() == thread_status::ok );
  REQUIRE( coro.coro_status() == coroutine_status::suspended );
  REQUIRE( coro.resume_safe() == valid );
  REQUIRE( coro.status() == thread_status::yield );
  REQUIRE( coro.coro_status() == coroutine_status::suspended );
  lua_expect<rstring> s = coro.resume_safe<rstring>();
  REQUIRE( coro.status() == thread_status::yield );
  REQUIRE( coro.coro_status() == coroutine_status::suspended );
  REQUIRE( s == "hello" );
  REQUIRE(
      coro.resume_safe<int>( 6 ) ==
      lua_unexpected<int>( "[string \"...\"]:6: some error" ) );
  REQUIRE( coro.status() == thread_status::err );
  REQUIRE( coro.coro_status() == coroutine_status::dead );
}

} // namespace
} // namespace lua
