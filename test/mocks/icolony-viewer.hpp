/****************************************************************
**icolony-viewer.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-13.
*
* Description: Mock instance of IColonyViewer.
*
*****************************************************************/
#pragma once

// Testing
#include "test/mocking.hpp"

// Revolution Now
#include "src/colony-view.hpp"
#include "src/ts.hpp"

// mock
#include "src/mock/mock.hpp"

namespace rn {

struct MockIColonyViewer : IColonyViewer {
  MOCK_METHOD( wait<e_colony_abandoned>, show, ( TS&, ColonyId ),
               () );
};

} // namespace rn
