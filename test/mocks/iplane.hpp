/****************************************************************
**iplane.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-26.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/plane.hpp"

// mock
#include "src/mock/mock.hpp"

// refl
#include "refl/to-str.hpp"

namespace rn {

/****************************************************************
** MockIPlane
*****************************************************************/
struct MockIPlane : IPlane {
  MOCK_METHOD( bool, will_handle_menu_click, ( e_menu_item ),
               () );
  MOCK_METHOD( void, handle_menu_click, ( e_menu_item ), () );
  MOCK_METHOD( void, on_logical_resolution_selected,
               ( gfx::e_resolution ), () );
  MOCK_METHOD( bool, supports_resolution, ( gfx::e_resolution ),
               ( const ) );
};

static_assert( !std::is_abstract_v<MockIPlane> );

} // namespace rn
