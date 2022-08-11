/****************************************************************
**irand.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-11.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/irand.hpp"

// mock
#include "src/mock/mock.hpp"

namespace rn {

struct MockIRand : IRand {
  MOCK_METHOD( bool, bernoulli, (double), () );
  MOCK_METHOD( int, between_ints, ( int, int, e_interval ), () );
  MOCK_METHOD( double, between_doubles, (double, double), () );
};

static_assert( !std::is_abstract_v<MockIRand> );

} // namespace rn
