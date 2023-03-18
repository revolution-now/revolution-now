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

// C++ standard library
#include <unordered_set>

using namespace std;

namespace rn {

unordered_set<e_menu_item> const kSupportedMenuItems{
    e_menu_item::exit,
    e_menu_item::save,
    e_menu_item::load,
    e_menu_item::revolution,
    e_menu_item::harbor_view,
    e_menu_item::cheat_explore_entire_map,
    e_menu_item::cheat_map_editor,
    e_menu_item::cheat_edit_fathers,
    e_menu_item::game_options,
};

/****************************************************************
** TurnPlane::Impl
*****************************************************************/
struct TurnPlane::Impl : public Plane {
  // State.
  co::stream<e_menu_item> menu_actions;

  vector<MenuPlane::Deregistrar> dereg;

  Impl( MenuPlane& menu_plane ) {
    for( e_menu_item item : kSupportedMenuItems )
      dereg.push_back(
          menu_plane.register_handler( item, *this ) );
  }

  bool covers_screen() const override { return false; }

  bool will_handle_menu_click( e_menu_item item ) override {
    return kSupportedMenuItems.contains( item );
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
