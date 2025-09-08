/****************************************************************
**imenu-handler.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-07.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/imenu-handler.hpp"

// mock
#include "src/mock/mock.hpp"

// refl
#include "refl/to-str.hpp"

namespace rn {

/****************************************************************
** MockIMenuHandler
*****************************************************************/
struct MockIMenuHandler : IMenuHandler {
  MOCK_METHOD( bool, will_handle_menu_click, ( e_menu_item ),
               () );
  MOCK_METHOD( void, handle_menu_click, ( e_menu_item ), () );
};

static_assert( !std::is_abstract_v<MockIMenuHandler> );

} // namespace rn
