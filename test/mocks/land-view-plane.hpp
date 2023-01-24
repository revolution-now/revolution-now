/****************************************************************
**land-view-plane.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-28.
*
* Description: Mock instance of ILandViewPlane.
*
*****************************************************************/
#pragma once

// Testing
#include "test/mocking.hpp"

// Revolution Now
#include "src/land-view.hpp"

// mock
#include "src/mock/mock.hpp"

// ss
#include "src/ss/colonies.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

namespace rn {

struct MockLandViewPlane : ILandViewPlane {
  MOCK_METHOD( void, set_visibility, (maybe<e_nation>), () );
  MOCK_METHOD( wait<>, ensure_visible, (Coord const&), () );
  MOCK_METHOD( wait<>, center_on_tile, ( Coord ), () );
  MOCK_METHOD( wait<>, ensure_visible_unit, ( GenericUnitId ),
               () );
  MOCK_METHOD( wait<LandViewPlayerInput_t>, get_next_input,
               ( UnitId ), () );
  MOCK_METHOD( wait<LandViewPlayerInput_t>, eot_get_next_input,
               (), () );
  MOCK_METHOD( wait<>, animate_move, ( UnitId, e_direction ),
               () );
  MOCK_METHOD( wait<>, animate_colony_depixelation,
               (Colony const&), () );
  MOCK_METHOD( wait<>, animate_unit_depixelation,
               (PixelationAnimation_t const&), () );
  MOCK_METHOD( wait<>, animate_attack,
               (GenericUnitId, GenericUnitId,
                std::vector<PixelationAnimation_t> const&, bool),
               () );
  MOCK_METHOD( wait<>, animate_colony_capture,
               ( UnitId, UnitId,
                 std::vector<PixelationAnimation_t> const&,
                 ColonyId ),
               () );
  MOCK_METHOD( void, reset_input_buffers, (), () );
  MOCK_METHOD( void, start_new_turn, (), () );
  MOCK_METHOD( void, zoom_out_full, (), () );
  MOCK_METHOD( maybe<UnitId>, unit_blinking, (), () );
  MOCK_METHOD( Plane&, impl, (), () );
};

} // namespace rn