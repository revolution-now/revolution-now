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

using namespace std;

namespace rn::co {

waitable<> any( vector<waitable<>> ws ) {
  waitable_promise<> wp;

  auto unified_callback =
      [wp]( waitable<>::value_type const& ) mutable {
        // It may be possible that both might finish and try to
        // set this value, so only allow the first one to do it.
        if( !wp.has_value() ) wp.set_value_emplace();
      };
  for( auto& w : ws )
    w.shared_state()->add_callback( unified_callback );

  auto waitable = wp.waitable();

  waitable.shared_state()->set_cancel( [ws]() mutable {
    // At this point, it may be the case, if we are in the middle
    // of cancelling a chain of coroutines, that the `waitable`
    // has been destroyed. In that case, the only references left
    // to `wp` will be held in the unified_callback, which are
    // held in the callbacks of the ws. So when we cancel the
    // last ws in this function (whichever one it is that is
    // still not ready and not cancelled) then its callbacks will
    // be cleared, the unified_callback will be destroyed, and
    // the `wp` will be destroyed, causing the shared_state that
    // holds this lambda function to be destroyed, along with the
    // lambda capture `ws`. So for that reason, and since we
    // don't really know which of the `ws` will trigger that (we
    // don't know if any have already been cancelled), we must
    // make a local copy on the stack, and then iterate over that
    // one. After we are done cancelling them all, this lambda
    // function may have been deleted, so we must not access the
    // captures thereafter.
    auto local_stack_ws = ws;
    for( auto& w : local_stack_ws ) w.cancel();
    // !! this lambda function and the shared_state that holds it
    // may be gone at this point.
  } );

  return waitable;
}

waitable<> any_cancel( vector<waitable<>> ws ) {
  co_await any( ws );
  // Need to cancel these directly instead of calling cancel on
  // the result of any because its cancel function will be
  // cleared once the value is set.
  for( auto& w : ws ) w.cancel();
}

waitable<> repeat_until_and_cancel(
    base::unique_func<waitable<>() const> get_repeatable,
    waitable<>                            until_this_finishes ) {
  auto repeater = [get_repeatable = std::move(
                       get_repeatable )]() -> waitable<> {
    while( true ) co_await get_repeatable();
  };
  co_await any_cancel( repeater(), until_this_finishes );
}

} // namespace rn::co
