/****************************************************************
**co-combinator.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-24.
*
* Description: Combinators for waitables.
*
*****************************************************************/
#include "co-combinator.hpp"

// Revolution Now
#include "error.hpp"
#include "waitable-coro.hpp"

using namespace std;

namespace rn::co {

/****************************************************************
** any
*****************************************************************/
waitable<> any( vector<waitable<>> ws ) {
  waitable_promise<> wp;
  for( auto& w : ws ) w.link_to_promise( wp );
  // !! Need to co_await instead of just returning the waitable<>
  // because we need to keep the ws alive (we own them now).
  co_await wp.waitable();
}

waitable<> any( waitable<>&& w ) {
  std::vector<waitable<>> v;
  v.push_back( std::move( w ) );
  return any( std::move( v ) );
}

waitable<> any( waitable<>&& w1, waitable<>&& w2 ) {
  std::vector<waitable<>> v;
  v.push_back( std::move( w1 ) );
  v.push_back( std::move( w2 ) );
  return any( std::move( v ) );
}

waitable<> any( waitable<>&& w1, waitable<>&& w2,
                waitable<>&& w3 ) {
  std::vector<waitable<>> v;
  v.push_back( std::move( w1 ) );
  v.push_back( std::move( w2 ) );
  v.push_back( std::move( w3 ) );
  return any( std::move( v ) );
}

/****************************************************************
** all
*****************************************************************/
waitable<> all( vector<waitable<>> ws ) {
  for( auto& w : ws ) co_await std::move( w );
}

waitable<> all( waitable<>&& w ) {
  std::vector<waitable<>> v;
  v.push_back( std::move( w ) );
  return all( std::move( v ) );
}

waitable<> all( waitable<>&& w1, waitable<>&& w2 ) {
  std::vector<waitable<>> v;
  v.push_back( std::move( w1 ) );
  v.push_back( std::move( w2 ) );
  return all( std::move( v ) );
}

waitable<> all( waitable<>&& w1, waitable<>&& w2,
                waitable<>&& w3 ) {
  std::vector<waitable<>> v;
  v.push_back( std::move( w1 ) );
  v.push_back( std::move( w2 ) );
  v.push_back( std::move( w3 ) );
  return all( std::move( v ) );
}

/****************************************************************
** repeat
*****************************************************************/
waitable<> repeat(
    base::unique_func<waitable<>() const> coroutine ) {
  while( true ) co_await coroutine();
}

} // namespace rn::co
