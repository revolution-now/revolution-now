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
  MOCK_METHOD( wait<>, landview_ensure_visible, (Coord const&),
               () );
  MOCK_METHOD( wait<>, landview_ensure_visible_unit, ( UnitId ),
               () );
  MOCK_METHOD( wait<LandViewPlayerInput_t>,
               landview_get_next_input, ( UnitId ), () );
  MOCK_METHOD( wait<LandViewPlayerInput_t>,
               landview_eot_get_next_input, (), () );
  MOCK_METHOD( wait<>, landview_animate_move,
               ( UnitId, e_direction ), () );
  MOCK_METHOD( wait<>, landview_animate_colony_depixelation,
               (Colony const&), () );
  MOCK_METHOD( wait<>, landview_animate_unit_depixelation,
               (UnitId, maybe<e_unit_type>), () );
  MOCK_METHOD( wait<>, landview_animate_attack,
               (UnitId, UnitId, bool), () );
  MOCK_METHOD( wait<>, landview_animate_colony_capture,
               ( UnitId, UnitId, ColonyId ), () );
  MOCK_METHOD( void, landview_reset_input_buffers, (), () );
  MOCK_METHOD( void, landview_start_new_turn, (), () );
  MOCK_METHOD( void, zoom_out_full, (), () );
  MOCK_METHOD( maybe<UnitId>, unit_blinking, (), () );
  MOCK_METHOD( Plane&, impl, (), () );
};

} // namespace rn