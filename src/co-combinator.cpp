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

using namespace std;

namespace rn {

waitable<> when_any( waitable<> w1, waitable<> w2 ) {
  waitable_promise<> wp;

  auto callback = [wp]( waitable<>::value_type const& ) mutable {
    // It may be possible that both might finish and try to set
    // this value, so only allow the first one to do it.
    if( !wp.has_value() ) wp.set_value_emplace();
  };
  w1.shared_state()->add_callback( callback );
  w2.shared_state()->add_callback( callback );

  return wp.get_waitable();
}

} // namespace rn
