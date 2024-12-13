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

// base
#include "base/macros.hpp"
#include "base/zero.hpp"

// C++ standard library
#include <string>

namespace rn {

struct IPlane;

enum class e_menu_item;

/****************************************************************
** MenuPlane
*****************************************************************/
struct MenuPlane {
  MenuPlane();
  ~MenuPlane();

  struct Deregistrar : base::zero<Deregistrar, e_menu_item> {
    using Base = base::zero<Deregistrar, e_menu_item>;

    Deregistrar() = default;

    Deregistrar( MenuPlane& menu_plane, IPlane& plane,
                 e_menu_item item )
      : Base( item ),
        menu_plane_( &menu_plane ),
        plane_( &plane ) {}

   private:
    MenuPlane* menu_plane_ = nullptr;
    IPlane* plane_         = nullptr;

    friend Base;
    // Implement base::zero.
    void free_resource();
  };

 public:
  [[nodiscard]] Deregistrar register_handler( e_menu_item item,
                                              IPlane& plane );

 private:
  void unregister_handler( e_menu_item item, IPlane& plane );

  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  IPlane& impl();
};

} // namespace rn
