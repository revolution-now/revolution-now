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
#include "error.hpp"
#include "stacktrace.hpp"

// base
#include "base/fs.hpp"

namespace testing {

fs::path const& data_dir();

// This literally default constructs all save-game data struc-
// tures. The result will not be an officially valid game state,
// but it may be ok for some unit tests.
void default_construct_all_game_state();

} // namespace testing
