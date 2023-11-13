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
  MOCK_METHOD( void, modify_entire_map_no_redraw,
               ( MapUpdateFunc ), () );
  MOCK_METHOD( std::vector<BuffersUpdated>, make_squares_visible,
               (e_nation, std::vector<Coord> const&), () );
  MOCK_METHOD( std::vector<BuffersUpdated>, make_squares_fogged,
               (e_nation, std::vector<Coord> const&), () );
  MOCK_METHOD( void, redraw, (), () );
  MOCK_METHOD( void, unrender, (), () );
};

static_assert( !std::is_abstract_v<MockIMapUpdater> );

} // namespace rn
