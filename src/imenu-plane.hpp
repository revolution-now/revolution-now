/****************************************************************
**imenu-plane.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-14.
*
* Description: Interface for accessing the menu server.
*
*****************************************************************/
#pragma once

// rds
#include "imenu-plane.rds.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

struct IPlane;

enum class e_menu_item;

/****************************************************************
** IMenuPlane
*****************************************************************/
struct IMenuPlane {
  virtual ~IMenuPlane() = default;

  virtual wait<maybe<e_menu_item>> open_menu(
      MenuLayout const& layout,
      MenuPosition const& position ) = 0;

  virtual IPlane& impl() = 0;
};

} // namespace rn
