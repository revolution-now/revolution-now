/****************************************************************
**rthread.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-26.
*
* Description: RAII holder for registry references to Lua
*              threads.
*
*****************************************************************/
#include "rthread.hpp"

// luapp
#include "c-api.hpp"

using namespace std;

namespace lua {

rthread::rthread( lua::cthread L, int ref ) : any( L, ref ) {
  lua::c_api C( L );
  C.registry_get( ref );
  CHECK( C.type_of( -1 ) == type::thread );
  // Replace the L held in this object by the one representing
  // the new thread (the L passed in as a parameter may represent
  // some other thread).
  this->L_ = C.tothread( -1 );
  CHECK( this->L_ != nullptr );
  C.pop();
}

base::maybe<rthread> lua_get( cthread L, int idx,
                              tag<rthread> ) {
  lua::c_api C( L );
  if( C.type_of( idx ) != type::thread ) return base::nothing;
  // Copy the requested value to the top of the stack.
  C.pushvalue( idx );
  return rthread( L, C.ref_registry() );
}

bool rthread::is_main() const noexcept {
  lua::c_api C( L_ );
  bool       is_main_thread = C.pushthread();
  C.pop();
  return is_main_thread;
}

lua_valid rthread::resetthread() const noexcept {
  lua::c_api C( L_ );
  return C.resetthread();
}

thread_status rthread::status() const noexcept {
  lua::c_api C( L_ );
  return C.status();
}

coroutine_status rthread::coro_status() const noexcept {
  lua::c_api C( L_ );
  return C.coro_status();
}

} // namespace lua
