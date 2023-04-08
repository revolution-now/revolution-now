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

#include "src/core-config.hpp"

// Revolution Now
#include "src/error.hpp"

// base
#include "src/base/fs.hpp" // FIXME

namespace testing {

fs::path const& data_dir();

} // namespace testing
