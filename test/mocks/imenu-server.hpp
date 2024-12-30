/****************************************************************
**imenu-server.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-25.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/imenu-server.hpp"
#include "src/plane.hpp"

// config
#include "src/config/menu-items.rds.hpp" // needed otherwise
                                         // e_menu{_item} args
                                         // can't be formatted.

// mock
#include "src/mock/mock.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

namespace rn {

/****************************************************************
** MockIMenuServer
*****************************************************************/
struct MockIMenuServer : IMenuServer {
  MOCK_METHOD( bool, can_handle_menu_click, ( e_menu_item ),
               ( const ) );
  MOCK_METHOD( bool, click_item, ( e_menu_item ), () );
  MOCK_METHOD( wait<maybe<e_menu_item>>, open_menu,
               (e_menu, MenuAllowedPositions const&), () );
  MOCK_METHOD( void, close_all_menus, (), () );
  MOCK_METHOD( void, show_menu_bar, (bool), () );
  MOCK_METHOD( void, enable_cheat_menu, (bool), () );
  MOCK_METHOD( IMenuServer::Deregistrar, register_handler,
               (e_menu_item, IPlane&), () );
  MOCK_METHOD( void, unregister_handler, (e_menu_item, IPlane&),
               () );

  IPlane& impl() override {
    static NoOpPlane plane;
    return plane;
  }
};

static_assert( !std::is_abstract_v<MockIMenuServer> );

} // namespace rn
