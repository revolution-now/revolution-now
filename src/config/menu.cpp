/****************************************************************
**menu.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-20.
*
# Description: Config data for the menus in the menu bar.
*
*****************************************************************/
#include "menu.hpp"

// config
#include "menu.rds.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

base::valid_or<string> config::menu::MenuLayout::validate()
    const {
  // Check that the menu has at least one non-divider item.
  int clickable_count = 0;
  for( auto const& item_conf : contents )
    if( item_conf.has_value() ) ++clickable_count;
  REFL_VALIDATE( clickable_count > 0,
                 "menu has no non-divider items." );

  return base::valid;
}

base::valid_or<string> config::menu::MenuConfig::validate()
    const {
  REFL_VALIDATE( !name.empty(), "menu name is empty." );

  // Check that the shortcut key is in the menu name.
  REFL_VALIDATE( name.find( shortcut ) != string_view::npos,
                 "menu `{}` does not contain shortcut key `{}`",
                 name, shortcut );

  return base::valid;
}

base::valid_or<string> config_menu_t::validate() const {
  // Check that all menus have unique shortcut keys.
  unordered_set<char> keys;
  for( auto menu : refl::enum_values<e_menu> ) {
    char const key = tolower( menus[menu].shortcut );
    REFL_VALIDATE( !keys.contains( key ),
                   "multiple menus have `{}` as a shortcut key "
                   "in either uppercase or lowercase form.",
                   key );
    keys.insert( key );
  }

  // Check that all menu items have unique names.
  unordered_set<string> names;
  for( e_menu_item const item :
       refl::enum_values<e_menu_item> ) {
    string const& name = items[item].name;
    REFL_VALIDATE( !names.contains( name ),
                   "multiple menu items have the name `{}`.",
                   name );
    names.insert( name );
  }

  // Check that all menu items are in precisely one menu.
  refl::enum_map<e_menu_item, bool> items;
  for( auto& [menu, conf] : menus ) {
    for( auto const& item_conf : layout[menu].contents )
      if( item_conf.has_value() ) {
        e_menu_item const item = *item_conf;
        REFL_VALIDATE(
            !items[item],
            "the menu item `{}` appears multiple times.", item );
        items[item] = true;
      }
  }
  for( auto item : refl::enum_values<e_menu_item> ) {
    REFL_VALIDATE( items[item],
                   "the menu item `{}` is not in any menu.",
                   item );
  }

  return base::valid;
}

} // namespace rn
