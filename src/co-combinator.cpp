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

  auto unified_callback =
      [wp]( waitable<>::value_type const& ) mutable {
        // It may be possible that both might finish and try to
        // set this value, so only allow the first one to do it.
        if( !wp.has_value() ) wp.set_value_emplace();
      };
  w1.shared_state()->add_callback( unified_callback );
  w2.shared_state()->add_callback( unified_callback );

  wp.shared_state()->set_cancel( [w1, w2]() mutable {
    // At this point either one (or both) of w1 and w2 are still
    // not ready. This is good, because if (and when) both of
    // them become ready or cancelled (as we will do before we
    // finish this function) then their callbacks will be
    // cleared, and the `unified_callback` above will get de-
    // stroyed, and thus the `wp` that it holds will get de-
    // stroyed. Then, if no one else is holding onto the waitable
    // that was produced from `wp`, that means that the shared
    // state associated with `wp` will go out of scope, which is
    // a problem because that shared state is what is holding
    // this lambda function and its captures. So we must be
    // careful that after both w1 and w2 are cancelled (or
    // ready), we must not do anything else in this function that
    // uses the captures (you can log though). So first we will
    // just get w1 and w2 onto the local stack so that we can
    // just call cancel on both of them without having to worry
    // about the first statement causing the captures in this
    // lambda to be destroyed (which generally will happen)
    // causing the second statement to crash.
    auto local_stack_w1 = w1;
    auto local_stack_w2 = w2;
    local_stack_w1.cancel();
    local_stack_w2.cancel();
    // !! this lambda function and the shared_state that holds it
    // may be gone at this point.
  } );

  return wp.waitable();
}

waitable<> when_any( std::vector<waitable<>> ws ) {
  return accumulate( ws.begin(), ws.end(), empty_waitable(),
                     [] λ( when_any( _1, _2 ) ) );
}

waitable<> when_any_with_cancel( waitable<> w1, waitable<> w2 ) {
  co_await when_any( w1, w2 );
  // Need to cancel these directly instead of calling cancel on
  // the result of when_any because its cancel function will be
  // cleared once the value is set.
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
