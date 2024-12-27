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
#include "imenu-server.hpp"
#include "plane.hpp"

// config
#include "config/menu-items.rds.hpp"

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
  e_menu_item::cheat_set_human_players,
  e_menu_item::cheat_kill_natives,
  e_menu_item::cheat_map_editor,
  e_menu_item::cheat_edit_fathers,
  e_menu_item::game_options,
};

/****************************************************************
** TurnPlane::Impl
*****************************************************************/
struct TurnPlane::Impl : public IPlane {
  // State.
  co::stream<e_menu_item> menu_actions;

  vector<IMenuServer::Deregistrar> dereg;

  Impl( IMenuServer& menu_server ) {
    for( e_menu_item item : kSupportedMenuItems )
      dereg.push_back(
          menu_server.register_handler( item, *this ) );
  }

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

  void on_logical_resolution_changed(
      gfx::e_resolution ) override {}
};

/****************************************************************
** TurnPlane
*****************************************************************/
IPlane& TurnPlane::impl() { return *impl_; }

TurnPlane::~TurnPlane() = default;

TurnPlane::TurnPlane( IMenuServer& menu_server )
  : impl_( new Impl( menu_server ) ) {}

wait<e_menu_item> TurnPlane::next_menu_action() {
  return impl_->next_menu_action();
}

} // namespace rn
