/****************************************************************
**menu.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-27.
*
* Description: Menu Bar
*
*****************************************************************/
#include "menu.hpp"

// Revolution Now
#include "aliases.hpp"
#include "config-files.hpp"
#include "errors.hpp"
#include "fonts.hpp"
#include "sdl-util.hpp"
#include "variant.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

using namespace std;

namespace rn {

namespace {

struct Menu {
  string name;
  bool   right_side;
  char   hot_key;
};

absl::flat_hash_map<e_menu, Menu> g_menus{
    {e_menu::game, {"Game", false, 'G'}},
    {e_menu::view, {"View", false, 'V'}},
    {e_menu::orders, {"Orders", false, 'O'}},
    {e_menu::pedia, {"Revolopedia", true, 'R'}}};

struct MenuDivider {};

// menu.cpp
struct MenuClickable {
  e_menu_item item;
  string      name;

  struct Callbacks {
    function<void( void )> on_click;
    function<bool( void )> enabled;
  };

  Callbacks callbacks;
};

using MenuItem = variant<MenuDivider, MenuClickable>;

#define ITEM( item, name )      \
  MenuClickable {               \
    e_menu_item::item, name, {} \
  }

#define DIVIDER \
  MenuDivider {}

absl::flat_hash_map<e_menu, Vec<MenuItem>> g_menu_def{
    {e_menu::game,
     {
         ITEM( about, "About" ), //
         DIVIDER,                //
         ITEM( exit, "Exit" )    //
     }},
    {e_menu::view,
     {
         ITEM( zoom_in, "Zoom In" ),          //
         ITEM( zoom_out, "Zoom Out" ),        //
         ITEM( restore_zoom, "Zoom Default" ) //
     }},
    {e_menu::orders,
     {
         ITEM( sentry, "Sentry" ),  //
         ITEM( fortify, "Fortify" ) //
     }},
    {e_menu::pedia,
     {
         ITEM( units_help, "Units" ) //
     }}};

absl::flat_hash_map<e_menu_item, MenuClickable*> g_menu_items;

absl::flat_hash_map<e_menu_item, e_menu> g_item_to_menu;

absl::flat_hash_map<e_menu, Vec<e_menu_item>> g_items_from_menu;

auto& on_click_handlers() {
  static absl::flat_hash_map<e_menu_item,
                             std::function<void( void )>>
      m;
  return m;
}

auto& is_enabled_handlers() {
  static absl::flat_hash_map<e_menu_item,
                             std::function<bool( void )>>
      m;
  return m;
}

struct ItemTextures {
  Texture normal;
  Texture highlighted;
  Texture disabled;
};

absl::flat_hash_map<e_menu_item, ItemTextures>
    g_menu_item_rendered;

struct MenuTextures {
  Texture      divider;
  Texture      item_background_normal;
  Texture      item_background_highlight;
  ItemTextures name;
  Texture      whole_menu_background;
};

absl::flat_hash_map<e_menu, MenuTextures> g_menu_rendered;

// Each menu may have a different width depending on the sizes of
// the elements inside of it, so we will render different sized
// dividers for each menu. All dividers in a given menu will be
// the same size though.
absl::flat_hash_map<e_menu, Texture> g_dividers;

auto background_active   = config_palette.yellow.sat1.lum11;
auto background_inactive = config_palette.orange.sat0.lum3;
auto foreground_active   = config_palette.orange.sat0.lum2;
auto foreground_inactive = config_palette.orange.sat1.lum11;
auto foreground_disabled = config_palette.grey.n44;

ItemTextures render_menu_element( string const&  s,
                                  optional<char> hot_key ) {
  ItemTextures res;
  // TODO
  (void)hot_key;
  res.normal = render_text_line_shadow( fonts::standard,
                                        foreground_inactive, s );

  res.highlighted = render_text_line_shadow(
      fonts::standard, foreground_active, s );

  res.disabled = render_text_line_shadow(
      fonts::standard, foreground_disabled, s );

  return res;
}

Delta compute_menu_items_delta( e_menu menu ) {
  Delta res;
  for( auto const& item : g_items_from_menu[menu] ) {
    CHECK( g_menu_item_rendered.contains( item ) );
    auto const& textures = g_menu_item_rendered[item];

    // These are probably all the same size, but just in case
    // they aren't...
    res = res.uni0n( textures.normal.size() );
    res = res.uni0n( textures.highlighted.size() );
    res = res.uni0n( textures.disabled.size() );
  }
  return res;
}

Texture render_divider( e_menu menu ) {
  auto    delta = compute_menu_items_delta( menu );
  Texture res   = create_texture( delta );
  clear_texture_transparent( res );
  // A divider is never highlighted.
  Color color = foreground_inactive;
  render_line( res, color, Coord{} + delta.h / 2 + 2_w,
               {delta.w - 4_w, 0_h} );
  return res;
}

Texture render_item_background( e_menu menu, bool highlight ) {
  auto    delta = compute_menu_items_delta( menu );
  Texture res   = create_texture( delta );
  auto    color =
      highlight ? background_active : background_inactive;
  render_fill_rect( res, color );
  return res;
}

Texture create_whole_menu_background( e_menu menu ) {
  // Include dividers
  int  num_elems = g_menu_def[menu].size();
  auto delta     = compute_menu_items_delta( menu );
  delta.h *= SY{num_elems};
  return create_texture( delta );
}

} // namespace

void register_menu_item_handler(
    e_menu_item                        item,
    std::function<void( void )> const& on_click,
    std::function<bool( void )> const& is_enabled ) {
  CHECK( on_click );
  CHECK( is_enabled );
  auto& on_clicks   = on_click_handlers();
  auto& is_enableds = is_enabled_handlers();
  CHECK( !on_clicks.contains( item ) );
  CHECK( !is_enableds.contains( item ) );
  on_clicks[item]   = on_click;
  is_enableds[item] = is_enabled;
}

void initialize_menus() {
  // Check that all menus have descriptors.
  for( auto menu : values<e_menu> ) {
    CHECK( g_menus.contains( menu ) );
    CHECK( g_menu_def.contains( menu ) );
  }

  // Populate the e_menu_item maps and verify no duplicates.
  for( auto& [menu, items] : g_menu_def ) {
    for( auto& item_desc : items ) {
      if( util::holds<MenuDivider>( item_desc ) ) continue;
      CHECK( util::holds<MenuClickable>( item_desc ) );
      auto& clickable = get<MenuClickable>( item_desc );
      g_items_from_menu[menu].push_back( clickable.item );
      CHECK( !g_menu_items.contains( clickable.item ) );
      g_menu_items[clickable.item] = &clickable;
      CHECK( !g_item_to_menu.contains( clickable.item ) );
      g_item_to_menu[clickable.item] = menu;
    }
  }

  // Check that all e_menu_items are in a menu.
  for( auto item : values<e_menu_item> ) {
    CHECK( g_menu_items.contains( item ) );
    CHECK( g_menu_items[item] != nullptr );
    CHECK( g_item_to_menu.contains( item ) );
  }

  // Populate Callbacks
  for( auto const& [item, func] : on_click_handlers() ) {
    CHECK( !g_menu_items[item]->callbacks.on_click );
    g_menu_items[item]->callbacks.on_click = func;
  }
  for( auto const& [item, func] : is_enabled_handlers() ) {
    CHECK( !g_menu_items[item]->callbacks.enabled );
    g_menu_items[item]->callbacks.enabled = func;
  }

  // Check that all e_menu_items have registered handlers.
  for( auto item : values<e_menu_item> ) {
    auto const& desc = *g_menu_items[item];
    CHECK( item == desc.item );
    CHECK( desc.name.size() > 0 );
    CHECK( desc.callbacks.on_click && desc.callbacks.enabled,
           "the menu item `{}` does not have callback handlers "
           "registered",
           item );
  }

  // Render Menu names
  for( auto menu_item : values<e_menu_item> )
    g_menu_item_rendered[menu_item] = render_menu_element(
        g_menu_items[menu_item]->name, nullopt );

  for( auto menu : values<e_menu> ) {
    g_menu_rendered[menu] = {
        render_divider( menu ),
        render_item_background( menu, /*highlight=*/false ),
        render_item_background( menu, /*highlight=*/true ),
        render_menu_element( g_menus[menu].name,
                             g_menus[menu].hot_key ),
        create_whole_menu_background( menu )};
  }

  // Render dividers
}

void cleanup_menus() {
  // This should free all the textures representing the menu
  // items.
  g_menu_rendered.clear();
  g_menu_item_rendered.clear();
}

function<void( void )> empty_handler = [] {};
function<bool( void )> enabled_true  = [] { return true; };

MENU_ITEM_HANDLER( about, empty_handler, enabled_true );
MENU_ITEM_HANDLER( exit, empty_handler, enabled_true );
MENU_ITEM_HANDLER( zoom_in, empty_handler, enabled_true );
MENU_ITEM_HANDLER( zoom_out, empty_handler, enabled_true );
MENU_ITEM_HANDLER( restore_zoom, empty_handler, enabled_true );
MENU_ITEM_HANDLER( sentry, empty_handler, enabled_true );
MENU_ITEM_HANDLER( fortify, empty_handler, enabled_true );
MENU_ITEM_HANDLER( units_help, empty_handler, enabled_true );

} // namespace rn
