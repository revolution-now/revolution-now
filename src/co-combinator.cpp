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
#include "logging.hpp"
#include "waitable-coro.hpp"

// base
#include "base/lambda.hpp"

// C++ stanard library
#include <numeric>

using namespace std;

namespace rn::co {

waitable<> when_any( waitable<> w1, waitable<> w2 ) {
  waitable_promise<> wp;

  auto callback = [wp]( waitable<>::value_type const& ) mutable {
    // It may be possible that both might finish and try to set
    // this value, so only allow the first one to do it.
    if( !wp.has_value() ) wp.set_value_emplace();
  };
  w1.shared_state()->add_callback( callback );
  w2.shared_state()->add_callback( callback );

  wp.shared_state()->set_cancel( [w1, w2]() mutable {
    w1.shared_state()->cancel();
    w2.shared_state()->cancel();
  } );

  return wp.get_waitable();
}

waitable<> when_any( std::vector<waitable<>> ws ) {
  return accumulate( ws.begin(), ws.end(), empty_waitable(),
                     [] λ( when_any( _1, _2 ) ) );
}

waitable<> when_any_with_cancel( waitable<> w1, waitable<> w2 ) {
  co_await when_any( w1, w2 );
  // Need to co_await on these directly instead of calling cancel
  // on the result of when_any because its cancel function will
  // be cleared once the value is set.
  w1.cancel();
  w2.cancel();
}

waitable<> when_any_with_cancel( std::vector<waitable<>> ws ) {
  return accumulate( ws.begin(), ws.end(), empty_waitable(),
                     [] λ( when_any_with_cancel( _1, _2 ) ) );
}

waitable<> repeat_until_and_cancel(
    base::unique_func<waitable<>() const> get_repeatable,
    waitable<>                            until_this_finishes ) {
  auto repeater = [get_repeatable = std::move(
                       get_repeatable )]() -> waitable<> {
    while( true ) co_await get_repeatable();
  };
  co_await when_any_with_cancel( repeater(),
                                 until_this_finishes );
}

} // namespace rn::co
