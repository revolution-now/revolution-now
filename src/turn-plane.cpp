/****************************************************************
**turn-plane.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-11.
*
* Description: A plane for the turn handler.
*
*****************************************************************/
#include "turn-plane.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "menu.hpp"
#include "plane.hpp"

using namespace std;

namespace rn {

/****************************************************************
** TurnPlane::Impl
*****************************************************************/
struct TurnPlane::Impl : public Plane {
  // State.
  co::stream<e_menu_item> menu_actions;

  MenuPlane::Deregistrar exit_dereg;
  MenuPlane::Deregistrar save_dereg;
  MenuPlane::Deregistrar load_dereg;
  MenuPlane::Deregistrar revolution_dereg;
  MenuPlane::Deregistrar harbor_view_dereg;
  MenuPlane::Deregistrar map_editor_dereg;

  Impl( MenuPlane& menu_plane ) {
    exit_dereg =
        menu_plane.register_handler( e_menu_item::exit, *this );
    save_dereg =
        menu_plane.register_handler( e_menu_item::save, *this );
    load_dereg =
        menu_plane.register_handler( e_menu_item::load, *this );
    revolution_dereg = menu_plane.register_handler(
        e_menu_item::revolution, *this );
    harbor_view_dereg = menu_plane.register_handler(
        e_menu_item::harbor_view, *this );
    map_editor_dereg = menu_plane.register_handler(
        e_menu_item::map_editor, *this );
  }

  bool covers_screen() const override { return false; }

  bool will_handle_menu_click( e_menu_item item ) override {
    switch( item ) {
      case e_menu_item::exit:
      case e_menu_item::save:
      case e_menu_item::load:
      case e_menu_item::revolution:
      case e_menu_item::harbor_view:
      case e_menu_item::map_editor: return true;
      default: break;
    }
    return false;
  }

  void handle_menu_click( e_menu_item item ) override {
    menu_actions.send( item );
  }

  wait<e_menu_item> next_menu_action() {
    e_menu_item item = co_await menu_actions.next();
    co_return item;
  }
};

/****************************************************************
** TurnPlane
*****************************************************************/
Plane& TurnPlane::impl() { return *impl_; }

TurnPlane::~TurnPlane() = default;

TurnPlane::TurnPlane( MenuPlane& menu_plane )
  : impl_( new Impl( menu_plane ) ) {}

wait<e_menu_item> TurnPlane::next_menu_action() {
  return impl_->next_menu_action();
}

} // namespace rn
