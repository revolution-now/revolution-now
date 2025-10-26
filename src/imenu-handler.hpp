/****************************************************************
**imenu-handler.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-07.
*
* Description: Interface to implement if one wants to handle menu
*              events (i.e., menu item selection).
*
*****************************************************************/
#pragma once

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
enum class e_menu_item;

/****************************************************************
** IMenuHandler
*****************************************************************/
struct IMenuHandler {
  virtual ~IMenuHandler() = default;

 public:
  // Returns true if and only if this object can handle this menu
  // item at this moment. This will only be called if this object
  // has registered itself as able to handle this menu item in
  // the first place. Will be used to control which menu items
  // are disabled. Note: the reason that this not const is be-
  // cause it allows implementations to use the same function to
  // implement this as for implementing handl_menu_click.
  //
  // TODO: once all the menus items are done, we should change
  // this interface so that it returns an enum that allows for
  // the possibility of saying "I normally can handle this item,
  // but it cannot be handled now and so do not display it in the
  // menu". This is so that e.g. when a free colonist is asking
  // for orders, the "Begin Trade Route" item does not appear in
  // the Orders menu, since that unit cannot start a trade route.
  // The OG does this and it has the effect of making the menus
  // easier to understand and more compact.
  [[nodiscard]] virtual bool will_handle_menu_click(
      e_menu_item item ) = 0;

  // Handle the click. This will only be called if this object
  // has registered itself as being able to handle this item and
  // has returned true for this item in will_handle_menu_click at
  // least once this frame.
  virtual void handle_menu_click( e_menu_item item ) = 0;
};

} // namespace rn
