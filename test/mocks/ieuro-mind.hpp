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
  MockIEuroMind( e_nation nation ) : IEuroMind( nation ) {}

  MOCK_METHOD( wait<e_declare_war_on_natives>,
               meet_tribe_ui_sequence, (MeetTribe const&), () );
};

static_assert( !std::is_abstract_v<MockIEuroMind> );

} // namespace rn
