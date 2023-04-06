/****************************************************************
**inative-mind.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-25.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/inative-mind.hpp"

// mock
#include "src/mock/mock.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

namespace rn {

/****************************************************************
** MockIRand
*****************************************************************/
struct MockINativeMind : INativeMind {
  MOCK_METHOD( NativeUnitId, select_unit,
               (std::set<NativeUnitId> const&), () );

  MOCK_METHOD( NativeUnitCommand, command_for, ( NativeUnitId ),
               () );
};

static_assert( !std::is_abstract_v<MockINativeMind> );

} // namespace rn
