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

  // In order for this to return true there needs to be at least
  // one plane registered to handle this item and the most recent
  // one to register needs to agree to handle it at the time that
  // this method is called.
  virtual bool can_handle_menu_click(
      e_menu_item item ) const = 0;

  // It is ok to call this on a menu item for which there is no
  // handler, even though it is not expected that will happen.
  // That is just to allow scenarios such as where the user
  // clicks an item and then while the click animation is run-
  // ning, something changes in another plane that causes the
  // item to not have a handler anymore. Returns true if the
  // click was actually made.
  virtual bool click_item( e_menu_item item ) = 0;

  virtual wait<maybe<e_menu_item>> open_menu(
      MenuContents const& contents ATTR_LIFETIMEBOUND,
      MenuAllowedPositions const& positions
          ATTR_LIFETIMEBOUND ) = 0;

 public: // Registration.
  struct Deregistrar : base::zero<Deregistrar, e_menu_item> {
    Deregistrar( IMenuPlane& menu_plane, IPlane& plane,
                 e_menu_item item );

   private:
    IMenuPlane* menu_plane_ = nullptr;
    IPlane* plane_          = nullptr;

    using Base = base::zero<Deregistrar, e_menu_item>;

    friend Base;
    // Implement base::zero.
    void free_resource();
  };

  [[nodiscard]] virtual Deregistrar register_handler(
      e_menu_item item, IPlane& plane ) = 0;

 private:
  virtual void unregister_handler( e_menu_item item,
                                   IPlane& plane ) = 0;

 public: // pimpl
  virtual IPlane& impl() = 0;
};

} // namespace rn
