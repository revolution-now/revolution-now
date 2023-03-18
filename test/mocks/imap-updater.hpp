/****************************************************************
**imap-updater.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-11.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/imap-updater.hpp"

// mock
#include "src/mock/mock.hpp"

// refl
#include "refl/to-str.hpp"

namespace rn {

/****************************************************************
** MockIMapUpdater
*****************************************************************/
struct MockIMapUpdater : IMapUpdater {
  MOCK_METHOD( BuffersUpdated, modify_map_square,
               ( Coord, SquareUpdateFunc ), () );
  MOCK_METHOD( void, modify_entire_map, ( MapUpdateFunc ), () );
  MOCK_METHOD( BuffersUpdated, make_square_visible,
               ( Coord, e_nation ), () );
  MOCK_METHOD( BuffersUpdated, make_square_fogged,
               ( Coord, e_nation ), () );
  MOCK_METHOD( void, redraw, (), () );
};

static_assert( !std::is_abstract_v<MockIMapUpdater> );

} // namespace rn
