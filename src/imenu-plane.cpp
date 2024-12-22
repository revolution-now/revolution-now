/****************************************************************
**imenu-plane.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-21.
*
* Description: Interface for accessing the menu server.
*
*****************************************************************/
#include "imenu-plane.hpp"

using namespace std;

namespace rn {

/****************************************************************
** IMenuPlane::Deregistrar
*****************************************************************/
IMenuPlane::Deregistrar::Deregistrar( IMenuPlane& menu_plane,
                                      IPlane& plane,
                                      e_menu_item item )
  : Base( item ), menu_plane_( &menu_plane ), plane_( &plane ) {}

void IMenuPlane::Deregistrar::free_resource() {
  CHECK( menu_plane_ != nullptr );
  CHECK( plane_ != nullptr );
  menu_plane_->unregister_handler( resource(), *plane_ );
}

} // namespace rn
