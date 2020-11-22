/****************************************************************
**testing.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-04.
*
* Description: Common definitions for unit tests.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "errors.hpp"
#include "stacktrace.hpp"

#define REQUIRE_THROWS_AS_RN( ... ) \
  REQUIRE_THROWS_AS( __VA_ARGS__, ::rn::exception_with_bt )

namespace rn::testing {

fs::path const& data_dir();

// This literally default constructs all save-game data struc-
// tures. The result will not be an officially valid game state,
// but it may be ok for some unit tests.
void default_construct_all_game_state();

} // namespace rn::testing
