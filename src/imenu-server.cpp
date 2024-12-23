/****************************************************************
**imenu-server.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-21.
*
* Description: Interface for accessing the menu server.
*
*****************************************************************/
#include "imenu-server.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "menu-body.hpp"

using namespace std;

namespace rn {

/****************************************************************
** IMenuServer::Deregistrar
*****************************************************************/
IMenuServer::Deregistrar::Deregistrar( IMenuServer& menu_server,
                                       IPlane& plane,
                                       e_menu_item item )
  : Base( item ),
    menu_server_( &menu_server ),
    plane_( &plane ) {}

void IMenuServer::Deregistrar::free_resource() {
  CHECK( menu_server_ != nullptr );
  CHECK( plane_ != nullptr );
  menu_server_->unregister_handler( resource(), *plane_ );
}

wait<maybe<e_menu_item>> IMenuServer::open_menu(
    e_menu const menu, MenuAllowedPositions const& positions ) {
  co_return co_await open_menu( contents_for_menu( menu ),
                                positions );
}

} // namespace rn
