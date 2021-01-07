/****************************************************************
**expect.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-24.
*
* Description: RN-specific expect utils.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// base
#include "base/expect.hpp"
#include "base/valid.hpp"

namespace rn {

// base::expect
using ::base::expect;
using ::base::expected;
using ::base::expected_ref;
using ::base::unexpected;

// base::valid_or
using ::base::invalid;
using ::base::valid;
using ::base::valid_or;
using ::base::valid_t;

} // namespace rn
