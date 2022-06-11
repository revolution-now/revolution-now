/****************************************************************
**menu.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-27.
*
* Description: Menu bar
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "co-combinator.hpp"

// Rds
#include "menu.rds.hpp"

// base
#include "base/macros.hpp"
#include "base/zero.hpp"

// C++ standard library
#include <string>

namespace rn {

struct Plane;

/****************************************************************
** MenuPlane
*****************************************************************/
struct MenuPlane {
  MenuPlane();
  ~MenuPlane();

  struct Deregistrar : base::zero<Deregistrar, e_menu_item> {
    using Base = base::zero<Deregistrar, e_menu_item>;

    Deregistrar() = default;

    Deregistrar( MenuPlane& menu_plane, Plane& plane,
                 e_menu_item item )
      : Base( item ),
        menu_plane_( &menu_plane ),
        plane_( &plane ) {}

   private:
    MenuPlane* menu_plane_ = nullptr;
    Plane*     plane_      = nullptr;

    friend Base;
    // Implement base::zero.
    void free_resource();
  };

 public:
  [[nodiscard]] Deregistrar register_handler( e_menu_item item,
                                              Plane& plane );

 private:
  void unregister_handler( e_menu_item item, Plane& plane );

  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl();
};

} // namespace rn
