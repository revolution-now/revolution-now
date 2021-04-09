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

waitable<> any( vector<waitable<>>& ws ) {
  waitable_promise<> wp;
  auto unified_callback = [wp]( waitable<>::value_type const& ) {
    wp.set_value_emplace_if_not_set();
  };
  for( auto& w : ws )
    w.shared_state()->add_callback( unified_callback );
  return wp.waitable();
}

waitable<> repeat(
    base::unique_func<waitable<>() const> coroutine ) {
  while( true ) co_await coroutine();
}

} // namespace rn::co
