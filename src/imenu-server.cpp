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

// config
#include "config/menu.rds.hpp"

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

// TODO: factor this out into a non-coroutine so that it can be
// tested.
wait<maybe<e_menu_item>> IMenuServer::open_menu(
    e_menu const menu, MenuAllowedPositions const& positions ) {
  auto const& conf = config_menu.layout[menu].contents;
  MenuContents contents;
  MenuItemGroup grp;
  auto collect_group = [&] {
    if( !grp.elems.empty() ) {
      contents.groups.push_back( grp );
      grp = {};
    }
  };
  for( auto const item : conf ) {
    if( !item.has_value() ) {
      collect_group();
      continue;
    }
    grp.elems.push_back( MenuElement::leaf{ .item = *item } );
  }
  collect_group();
  co_return co_await open_menu( contents, positions );
}

} // namespace rn
