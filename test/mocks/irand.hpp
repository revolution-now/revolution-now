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

// base
#include "refl/to-str.hpp" // Needed otherwise the mock methods
                           // won't see the reflected enums as
                           // being formattable and will format
                           // them as "?".

namespace rn {

/****************************************************************
** MockIRand
*****************************************************************/
struct MockIRand : IRand {
  MOCK_METHOD( void, reseed, (rng::entropy const&), () );
  MOCK_METHOD( bool, bernoulli, (double), () );
  MOCK_METHOD( int, uniform_int, (int, int), () );
  MOCK_METHOD( double, uniform_double, (double, double), () );
  MOCK_METHOD( rng::entropy, generate_deterministic_seed, (),
               () );
};

static_assert( !std::is_abstract_v<MockIRand> );

/****************************************************************
** Helpers
*****************************************************************/
// This will take a vector of indices representing how we'd like
// a vector shuffled and will generate all of the mock expect
// calls to make that happen.
void expect_shuffle( MockIRand& rand,
                     std::vector<int> const& indices );

} // namespace rn
