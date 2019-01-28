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
#include "globals.hpp"
#include "logging.hpp"
#include "plane.hpp"
#include "sdl-util.hpp"
#include "variant.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** Main Data Structures
*****************************************************************/
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
         ITEM( about, "About this Game" ),  //
         DIVIDER,                           //
         ITEM( revolution, "REVOLUTION!" ), //
         DIVIDER,                           //
         ITEM( retire, "Retire" ),          //
         ITEM( exit, "Exit to \"DOS\"" )    //
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

/****************************************************************
** Menu State
*****************************************************************/
// clang-format off
struct none {};
struct hover { e_menu_item item; };
// clang-format on

using item_hover = variant<none, hover>;

// clang-format off
struct open      { e_menu      menu;
                   item_hover  hover; };
struct opening   { e_menu      menu;
                   double      percent; };
struct clicking  { e_menu_item item;
                   int         flicker; };
struct closing   { e_menu      menu;
                   double      percent; };
struct switching { e_menu      from;
                   e_menu      to;
                   double      percent; };
// clang-format on

using menu_state =
    variant<open, opening, switching, clicking, closing>;

// clang-format off
struct menus_off    {};
struct menus_closed {};
struct menu_open    { menu_state state; };
// clang-format on

using menus_state = variant<menus_off, menus_closed, menu_open>;

menus_state g_menu_state{menu_open{
    open{e_menu::game, hover{e_menu_item::revolution}}}};

// Opt<e_menu> open_menu() { return e_menu::view; }

// Opt<e_menu_item> selected_item() {
//  auto res = e_menu_item::zoom_out;
//  CHECK( g_item_to_menu[res] == open_menu() );
//  return res;
//}

/****************************************************************
** Rendering Implmementation
*****************************************************************/
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
  Texture      menu_background_normal;
  Texture      menu_background_highlight;
};

absl::flat_hash_map<e_menu, MenuTextures> g_menu_rendered;

Texture menu_bar_tx;

// Each menu may have a different width depending on the sizes of
// the elements inside of it, so we will render different sized
// dividers for each menu. All dividers in a given menu will be
// the same size though.
absl::flat_hash_map<e_menu, Texture> g_dividers;

auto background_active = [] {
  return config_palette.yellow.sat1.lum11;
};
auto background_inactive = [] {
  return config_palette.orange.sat0.lum3;
};
auto const& background_menu_inactive = [] {
  static auto val = background_inactive().shaded().shaded();
  return val;
};
auto foreground_active = [] {
  return config_palette.orange.sat0.lum2;
};
auto foreground_inactive = [] {
  return config_palette.orange.sat1.lum11;
};
auto foreground_disabled = [] {
  return config_palette.grey.n44;
};

Delta compute_menus_delta() {
  Delta res;
  for( auto menu : values<e_menu> ) {
    CHECK( g_menu_rendered.contains( menu ) );
    auto const& textures = g_menu_rendered[menu];
    CHECK( textures.name.normal );
    res = res.uni0n( textures.name.normal.size() );
  }
  return res;
}

constexpr W first_menu_start{2};
constexpr W menu_padding{2};
constexpr W menu_spacing{8};

X menu_display_x_pos( e_menu target ) {
  X pos{0};
  pos += first_menu_start;
  for( auto menu : values<e_menu> ) {
    // TODO: is menu visible
    CHECK( g_menu_rendered.contains( menu ) );
    auto const& textures = g_menu_rendered[menu];
    if( menu == target ) return pos;
    pos += textures.menu_background_normal.size().w;
    pos += menu_spacing;
  }
  SHOULD_NOT_BE_HERE;
}

Texture create_menu_bar_texture() {
  auto  delta_screen = logical_screen_pixel_dimensions();
  auto  delta_menus  = compute_menus_delta();
  Delta res{delta_screen.w, delta_menus.h};
  return create_texture( res );
}

ItemTextures render_menu_element( string const&  s,
                                  optional<char> hot_key ) {
  logger->info( "rendering `{}`", s );
  ItemTextures res;
  // TODO
  (void)hot_key;
  res.normal = render_text_line_shadow(
      fonts::standard, foreground_inactive(), s );

  res.highlighted = render_text_line_shadow(
      fonts::standard, foreground_active(), s );

  res.disabled = render_text_line_shadow(
      fonts::standard, foreground_disabled(), s );

  return res;
}

Delta compute_menu_items_delta( e_menu menu ) {
  Delta res;
  for( auto const& item : g_items_from_menu[menu] ) {
    CHECK( g_menu_item_rendered.contains( item ) );
    auto const& textures = g_menu_item_rendered[item];

    // These are probably all the same size, but just in case
    // they aren't...
    res = res.uni0n( textures.normal.size() +
                     menu_padding * 2_sx );
    res = res.uni0n( textures.highlighted.size() +
                     menu_padding * 2_sx );
    res = res.uni0n( textures.disabled.size() +
                     menu_padding * 2_sx );
  }
  return res;
}

Texture render_divider( e_menu menu ) {
  auto    delta = compute_menu_items_delta( menu );
  Texture res   = create_texture( delta );
  clear_texture_transparent( res );
  // A divider is never highlighted.
  Color color = foreground_inactive();
  render_line( res, color, Coord{} + delta.h / 2 + 2_w,
               {delta.w - 4_w, 0_h} );
  return res;
}

Texture render_item_background( e_menu menu, bool highlight ) {
  auto    delta = compute_menu_items_delta( menu );
  Texture res   = create_texture( delta );
  auto    color =
      highlight ? background_active() : background_inactive();
  fill_texture( res, color );
  return res;
}

Texture render_menu_name_background( e_menu menu,
                                     bool   highlight ) {
  CHECK( g_menu_rendered.contains( menu ) );
  CHECK( g_menu_rendered[menu].name.normal );
  auto delta = g_menu_rendered[menu].name.normal.size();
  delta += menu_padding * 2_sx;
  Texture res   = create_texture( delta );
  auto    color = highlight ? background_active()
                         : background_menu_inactive();
  fill_texture( res, color );
  return res;
}

Texture create_whole_menu_background( e_menu menu ) {
  // Include dividers
  int  num_elems = g_menu_def[menu].size();
  auto delta     = compute_menu_items_delta( menu );
  delta.h *= SY{num_elems};
  return create_texture( delta );
}

Texture const& render_open_menu( e_menu           menu,
                                 Opt<e_menu_item> highlighted ) {
  CHECK( g_menu_rendered.contains( menu ) );
  if( highlighted.has_value() ) {
    CHECK( g_item_to_menu[*highlighted] == menu );
  }
  auto const& textures = g_menu_rendered[menu];
  auto&       dst      = textures.whole_menu_background;
  Coord       pos{};
  H           height = textures.item_background_normal.size().h;
  for( auto const& item_desc : g_menu_def[menu] ) {
    auto matcher = scelta::match(
        [&]( MenuDivider ) {
          copy_texture( textures.item_background_normal, dst,
                        pos );
          copy_texture( textures.divider, dst, pos );
        },
        [&]( MenuClickable const& clickable ) {
          auto const& desc = g_menu_items[clickable.item];
          auto const& rendered =
              g_menu_item_rendered[clickable.item];
          Texture const* from =
              !desc->callbacks.enabled()
                  ? &rendered.disabled
                  : ( clickable.item == highlighted )
                        ? &rendered.highlighted
                        : &rendered.normal;
          Texture const* background =
              ( clickable.item == highlighted )
                  ? &textures.item_background_highlight
                  : &textures.item_background_normal;
          copy_texture( *background, dst, pos );
          copy_texture( *from, dst, pos + menu_padding );
        } );

    matcher( item_desc );
    pos += height;
  }

  return dst;
}

void render_menu_bar() {
  CHECK( menu_bar_tx );
  fill_texture( menu_bar_tx, background_menu_inactive() );
  for( auto menu : values<e_menu> ) {
    CHECK( g_menu_rendered.contains( menu ) );
    auto const& textures = g_menu_rendered[menu];
    using Txs            = pair<Texture const*, Texture const*>;
    // Given `menu`, this matcher visits the global menu state
    // and returns a foreground/background texture pair for that
    // menu.
    auto matcher = scelta::match(
        []( menus_off ) { return Txs{}; },
        [&]( menus_closed ) {
          return pair{&textures.name.normal,
                      &textures.menu_background_normal};
        },
        [&]( menu_open const& mo ) {
          auto matcher = scelta::match(
              [&]( open const& o ) {
                if( o.menu == menu )
                  return pair{
                      &textures.name.highlighted,
                      &textures.menu_background_highlight};

                else
                  return pair{&textures.name.normal,
                              &textures.menu_background_normal};
              },
              []( opening ) { return Txs{}; },
              []( switching ) { return Txs{}; },
              []( clicking ) { return Txs{}; },
              []( closing ) { return Txs{}; } );
          return matcher( mo.state );
        } );
    auto [text, back] = matcher( g_menu_state );
    if( text && back ) {
      auto pos = menu_display_x_pos( menu );
      copy_texture( *back, menu_bar_tx, {0_y, pos} );
      copy_texture( *text, menu_bar_tx,
                    {0_y, pos + menu_padding} );
    }
  }
}

void display_menu_bar_tx( Texture const& tx ) {
  render_menu_bar();
  CHECK( tx.size().w == menu_bar_tx.size().w );
  copy_texture( menu_bar_tx, tx, Coord{} );
}

} // namespace

/****************************************************************
** Top-level Render Method
*****************************************************************/
void render_menus( Texture const& tx ) {
  display_menu_bar_tx( tx );
  auto maybe_render_open_menu = scelta::match(
      []( menus_off ) {},     //
      [&]( menus_closed ) {}, //
      [&]( menu_open const& mo ) {
        auto matcher = scelta::match(
            [&]( open const& o ) {
              Coord pos = {0_y + menu_bar_tx.size().h,
                           menu_display_x_pos( o.menu )};

              Opt<e_menu_item> highlighted;
              if_v( o.hover, hover, val ) {
                highlighted = val->item;
              }
              auto const& open_tx =
                  render_open_menu( o.menu, highlighted );
              copy_texture( open_tx, tx, pos );
            },
            []( opening ) {},   //
            []( switching ) {}, //
            []( clicking ) {},  //
            []( closing ) {}    //
        );
        return matcher( mo.state );
      } //
  );
  maybe_render_open_menu( g_menu_state );
}

/****************************************************************
** Registration
*****************************************************************/
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

/****************************************************************
** Initialization
*****************************************************************/
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
    g_menu_rendered[menu]         = {};
    g_menu_rendered[menu].divider = render_divider( menu );
    g_menu_rendered[menu].item_background_normal =
        render_item_background( menu, /*hightlight=*/false );
    g_menu_rendered[menu].item_background_highlight =
        render_item_background( menu, /*hightlight=*/true );
    g_menu_rendered[menu].name = render_menu_element(
        g_menus[menu].name, g_menus[menu].hot_key );
    g_menu_rendered[menu].whole_menu_background =
        create_whole_menu_background( menu );
    g_menu_rendered[menu].menu_background_normal =
        render_menu_name_background( menu, /*highlight=*/false );
    g_menu_rendered[menu].menu_background_highlight =
        render_menu_name_background( menu, /*highlight=*/true );
  }

  menu_bar_tx = create_menu_bar_texture();
}

void cleanup_menus() {
  // This should free all the textures representing the menu
  // items.
  g_menu_rendered.clear();
  g_menu_item_rendered.clear();
}

/****************************************************************
** Handlers (temporary)
*****************************************************************/
function<void( void )> empty_handler = [] {};
function<bool( void )> enabled_true  = [] { return true; };

MENU_ITEM_HANDLER( about, empty_handler, enabled_true );
MENU_ITEM_HANDLER( revolution, empty_handler, enabled_true );
MENU_ITEM_HANDLER( retire, empty_handler, enabled_true );
MENU_ITEM_HANDLER( exit, empty_handler, enabled_true );
MENU_ITEM_HANDLER( zoom_in, empty_handler, enabled_true );
MENU_ITEM_HANDLER( zoom_out, empty_handler, enabled_true );
MENU_ITEM_HANDLER( restore_zoom, empty_handler, enabled_true );
MENU_ITEM_HANDLER( sentry, empty_handler, enabled_true );
MENU_ITEM_HANDLER( fortify, empty_handler, enabled_true );
MENU_ITEM_HANDLER( units_help, empty_handler, enabled_true );

/****************************************************************
** The Menu Plane
*****************************************************************/
struct MenuPlane : public Plane {
  MenuPlane() = default;
  bool enabled() const override { return true; }
  bool covers_screen() const override { return false; }
  void draw( Texture const& tx ) const override {
    clear_texture_transparent( tx );
    render_menus( tx );
  }
  bool input( input::event_t const& event ) override {
    auto matcher = scelta::match(
        []( input::unknown_event_t ) { return false; },
        []( input::quit_event_t ) { return false; },
        []( input::key_event_t const& key_event ) {
          if( key_event.change == input::e_key_change::down ) {
            switch( key_event.keycode ) {
              case ::SDLK_LEFT: return true;
              case ::SDLK_RIGHT: return true;
              case ::SDLK_UP: return true;
              case ::SDLK_DOWN: return true;
              default: return false;
            }
          }
          return false;
        },
        []( input::mouse_wheel_event_t ) { return false; },
        []( input::mouse_move_event_t ) { return false; },
        []( input::mouse_button_event_t ) { return false; },
        []( input::mouse_drag_event_t ) { return false; } );
    return matcher( event );
  }
};

MenuPlane g_menu_plane;

Plane* menu_plane() { return &g_menu_plane; }

} // namespace rn
