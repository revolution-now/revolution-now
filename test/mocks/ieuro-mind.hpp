/****************************************************************
**ieuro-mind.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-31.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/ieuro-mind.hpp"

// mock
#include "src/mock/mock.hpp"

// refl
#include "refl/to-str.hpp"

namespace rn {

/****************************************************************
** MockIRand
*****************************************************************/
struct MockIEuroMind : IEuroMind {
  MOCK_METHOD( e_nation, nation, (), ( const ) );
};

static_assert( !std::is_abstract_v<MockIEuroMind> );

} // namespace rn
