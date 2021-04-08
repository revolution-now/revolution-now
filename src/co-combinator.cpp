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
#include "rand.hpp"
#include "waitable-coro.hpp"

// base
#include "base/lambda.hpp"

using namespace std;

namespace rn::co {

namespace {

// Returns a waitable that will be ready when at least one of the
// waitables becomes ready. The other waitables may still be run-
// ning (i.e., they will be left detached). If the vector is
// empty it will be in a ready state.
waitable<> any_detach( vector<waitable<>> ws ) {
  waitable_promise<> wp;

  auto unified_callback = [wp]( waitable<>::value_type const& ) {
    // It may be possible that both might finish and try to
    // set this value, so only allow the first one to do it.
    if( !wp.has_value() ) wp.finish();
  };
  for( auto& w : ws )
    w.shared_state()->add_callback( unified_callback );

  waitable<> res = wp.waitable();
  res.shared_state()->set_cancel( [ws]() {
    // When we are in this lambda, it is possible that the only
    // references left to the shared_state that holds it is in-
    // side the `ws`. This can happen when both the promise and
    // waitables associated with this shared state have all been
    // destroyed, as can happen while cancelling a chain of
    // coroutines. Then how could this lambda be called? It could
    // be called through a weak reference to the shared_state
    // added in the await_suspend method. So when we cancel the
    // last `ws` in this function (whichever one it is that is
    // still not ready and not cancelled) then its callbacks will
    // be cleared and this shared_state could be released,
    // thereby releasing this lambda function itself as well as
    // its capture.s So for that reason, and since we don't re-
    // ally know which of the `ws` will trigger that (we don't
    // know if any have already been cancelled), we must make a
    // local copy on the stack, and then iterate over that one.
    // After we are done cancelling them all, this lambda func-
    // tion may have been deleted, so we must not access the cap-
    // tures thereafter.
    vector<waitable<>> local_stack_ws = ws;
    WHEN_DEBUG( rng::shuffle( local_stack_ws ) );
    for( auto& w : local_stack_ws ) w.cancel();
    // !! this lambda function and the shared_state that holds it
    // may be gone at this point.
  } );
  return res;
}

} // namespace

waitable<> any( vector<waitable<>> ws ) {
  SCOPE_EXIT( for( auto& w : ws ) w.cancel() );
  co_await any_detach( ws );
}

waitable<> all( vector<waitable<>> ws ) {
  SCOPE_EXIT( for( auto& w : ws ) w.cancel() );
  for( auto& w : ws ) co_await w;
}

waitable<> UntilDo::operator()( waitable<> what,
                                waitable<> background ) const {
  SCOPE_EXIT( background.cancel() );
  // Can't do a `return` here because of the SCOPE_EXIT above.
  co_await what;
}

waitable<> repeat(
    base::unique_func<waitable<>() const> coroutine ) {
  while( true ) co_await coroutine();
}

} // namespace rn::co
