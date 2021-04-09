/****************************************************************
**co-combinator.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-24.
*
* Description: Combinators for waitables.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "error.hpp"
#include "waitable-coro.hpp"

// base
#include "base/unique-func.hpp"

// C++ standard library
#include <vector>

namespace rn::co {

// Returns a waitable that will be ready when (and as soon as)
// the first waitable becomes ready. The others may still be run-
// ning. We don't accept temporaries to remind the user that this
// function won't keep waitables alive.
waitable<> any( std::vector<waitable<>>& ws );

// Only take lvalue ref because this function won't keep tempo-
// raries alive.
template<typename... Waitables>
waitable<> any( Waitables&... ws ) {
  std::vector<waitable<>> v{ ws... };
  return any( v );
}

waitable<> repeat(
    base::unique_func<waitable<>() const> coroutine );

} // namespace rn::co
