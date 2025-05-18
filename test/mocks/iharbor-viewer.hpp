/****************************************************************
**iharbor-viewer.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-18.
*
* Description: Mock instance of IHarborViewer.
*
*****************************************************************/
#pragma once

// Testing
#include "test/mocking.hpp"

// Revolution Now
#include "src/harbor-view.hpp"
#include "src/ts.hpp"

// mock
#include "src/mock/mock.hpp"

namespace rn {

struct MockIHarborViewer : IHarborViewer {
  MOCK_METHOD( void, set_selected_unit, ( UnitId ), () );
  MOCK_METHOD( wait<>, show, (), () );
};

static_assert( !std::is_abstract_v<MockIHarborViewer> );

} // namespace rn
