/****************************************************************
**igoto-viewer.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-07.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/igoto-viewer.hpp"

// mock
#include "src/mock/mock.hpp"

namespace rn {

/****************************************************************
** MockIGotoMapViewer
*****************************************************************/
struct MockIGotoMapViewer : IGotoMapViewer {
  MOCK_METHOD( bool, can_enter_tile, ( gfx::point ), ( const ) );
  MOCK_METHOD( e_map_side, map_side, ( gfx::point ), ( const ) );
  MOCK_METHOD( e_map_side_edge, is_on_map_side_edge,
               ( gfx::point ), ( const ) );
  MOCK_METHOD( maybe<bool>, is_sea_lane, ( gfx::point ),
               ( const ) );
  MOCK_METHOD( maybe<bool>, has_lcr, ( gfx::point ), ( const ) );
  MOCK_METHOD( bool, has_colony, ( gfx::point ), ( const ) );
  MOCK_METHOD( bool, ends_turn_in_colony, (), ( const ) );
  MOCK_METHOD( maybe<MovementPoints>, movement_points_required,
               ( gfx::point, e_direction ), ( const ) );
  MOCK_METHOD( MovementPoints, minimum_heuristic_tile_cost, (),
               ( const ) );
};

static_assert( !std::is_abstract_v<MockIGotoMapViewer> );

} // namespace rn
