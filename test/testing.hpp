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

#define REQUIRE_THROWS_AS_RN( ... ) \
  REQUIRE_THROWS_AS( __VA_ARGS__, ::rn::exception_with_bt )

namespace rn {} // namespace rn
