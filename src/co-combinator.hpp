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
#include "waitable.hpp"

namespace rn {

// Returns a waitable that will be ready when at least one of the
// arguments becomes ready. That means that it won't wait for
// both of them; the one that is not yet ready will either need
// to be awaited upon itself, or cancelled before reusing it.
waitable<> when_any( waitable<> w1, waitable<> w2 );

} // namespace rn
