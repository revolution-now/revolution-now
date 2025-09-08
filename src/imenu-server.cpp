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

using namespace std;

namespace rn {

/****************************************************************
** IMenuServer::Deregistrar
*****************************************************************/
IMenuServer::Deregistrar::Deregistrar( IMenuServer& menu_server,
                                       IMenuHandler& handler,
                                       e_menu_item item )
  : Base( item ),
    menu_server_( &menu_server ),
    handler_( &handler ) {}

void IMenuServer::Deregistrar::free_resource() {
  CHECK( menu_server_ != nullptr );
  CHECK( handler_ != nullptr );
  menu_server_->unregister_handler( resource(), *handler_ );
}

} // namespace rn
