/****************************************************************
**co-combinator.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-24.
*
* Description: Combinators for waits.
*
*****************************************************************/
#include "co-combinator.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "error.hpp"

// base
#include "base/scope-exit.hpp"

using namespace std;

namespace rn::co {

/****************************************************************
** any
*****************************************************************/
wait<> any( vector<wait<>> ws ) {
  wait_promise<> wp;
  for( auto& w : ws ) disjunctive_link_to_promise( w, wp );
  // We need this because if this coroutine gets stopped by an
  // exception or cancelled while co-awaiting below then we need
  // to ensure that all of the waits have their callbacks
  // cleared, because the callbacks will be holding a reference
  // to the above promise which will be out of scope, which would
  // cause a crash if someone subsequently fulfilled another
  // promise in the list.
  SCOPE_EXIT( for( auto& w : ws ) w.cancel() );
  // !! Need to co_await instead of just returning the wait<>
  // because we need to keep the ws alive (we own them now).
  co_await wp.wait();
}

wait<> any( wait<>&& w ) {
  std::vector<wait<>> v;
  v.push_back( std::move( w ) );
  return any( std::move( v ) );
}

wait<> any( wait<>&& w1, wait<>&& w2 ) {
  std::vector<wait<>> v;
  v.push_back( std::move( w1 ) );
  v.push_back( std::move( w2 ) );
  return any( std::move( v ) );
}

wait<> any( wait<>&& w1, wait<>&& w2, wait<>&& w3 ) {
  std::vector<wait<>> v;
  v.push_back( std::move( w1 ) );
  v.push_back( std::move( w2 ) );
  v.push_back( std::move( w3 ) );
  return any( std::move( v ) );
}

/****************************************************************
** all
*****************************************************************/
wait<> all( vector<wait<>> ws ) {
  for( auto& w : ws ) co_await std::move( w );
}

wait<> all( wait<>&& w ) {
  std::vector<wait<>> v;
  v.push_back( std::move( w ) );
  return all( std::move( v ) );
}

wait<> all( wait<>&& w1, wait<>&& w2 ) {
  std::vector<wait<>> v;
  v.push_back( std::move( w1 ) );
  v.push_back( std::move( w2 ) );
  return all( std::move( v ) );
}

wait<> all( wait<>&& w1, wait<>&& w2, wait<>&& w3 ) {
  std::vector<wait<>> v;
  v.push_back( std::move( w1 ) );
  v.push_back( std::move( w2 ) );
  v.push_back( std::move( w3 ) );
  return all( std::move( v ) );
}

/****************************************************************
** loop
*****************************************************************/
wait<> loop( base::unique_func<wait<>() const> coroutine ) {
  while( true ) co_await coroutine();
}

} // namespace rn::co
