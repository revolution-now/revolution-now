/****************************************************************
**co-time.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-21.
*
* Description: Time-related wait and coroutine helpers.
*
*****************************************************************/
#include "co-time.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "frame.hpp"
#include "lua-wait.hpp"
#include "lua.hpp"

// luapp
#include "luapp/register.hpp"
#include "luapp/state.hpp"

using namespace std;

namespace rn {

wait<std::chrono::microseconds> co_await_transform(
    chrono::microseconds us ) {
  return wait_for_duration( us );
}

wait<chrono::microseconds> wait_for_duration(
    chrono::microseconds us ) {
  if( us == chrono::microseconds{ 0 } )
    return wait<chrono::microseconds>( 0 );
  wait_promise<chrono::microseconds> p;
  auto                               now = Clock_t::now();
  auto after_time = [p, then = now]() mutable {
    p.set_value( duration_cast<chrono::microseconds>(
        Clock_t::now() - then ) );
  };
  subscribe_to_frame_tick( after_time, us, /*repeating=*/false );
  return p.wait();
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( wait_for_micros, wait<int>, int micros ) {
  chrono::microseconds actual = co_await wait_for_duration(
      chrono::microseconds{ micros } );
  co_return actual.count();
}

} // namespace

} // namespace rn
