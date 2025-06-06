/****************************************************************
**game-options.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-12.
*
* Description: Implements the game options dialog box(es).
*
*****************************************************************/
#include "game-options.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "igui.hpp"
#include "imap-updater.hpp"
#include "plane-stack.hpp"
#include "ts.hpp"
#include "views.hpp"
#include "window.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"

using namespace std;

namespace rn {

namespace {

// Should only be called when the option value changes.
void on_option_enabled( TS& ts, e_game_menu_option option ) {
  lg.info( "enabling game option {}.", option );
  switch( option ) {
    case e_game_menu_option::show_indian_moves:
      break;
    case e_game_menu_option::show_foreign_moves:
      break;
    case e_game_menu_option::fast_piece_slide:
      break;
    case e_game_menu_option::end_of_turn:
      break;
    case e_game_menu_option::autosave:
      break;
    case e_game_menu_option::combat_analysis:
      break;
    case e_game_menu_option::water_color_cycling:
      break;
    case e_game_menu_option::tutorial_hints:
      break;
    case e_game_menu_option::show_fog_of_war:
      ts.map_updater().mutate_options_and_redraw(
          [&]( MapUpdaterOptions& options ) {
            options.render_fog_of_war = true;
          } );
      break;
  }
}

// Should only be called when the option value changes.
void on_option_disabled( TS& ts, e_game_menu_option option ) {
  lg.info( "disabling game option {}.", option );
  switch( option ) {
    case e_game_menu_option::show_indian_moves:
      break;
    case e_game_menu_option::show_foreign_moves:
      break;
    case e_game_menu_option::fast_piece_slide:
      break;
    case e_game_menu_option::end_of_turn:
      break;
    case e_game_menu_option::autosave:
      break;
    case e_game_menu_option::combat_analysis:
      break;
    case e_game_menu_option::water_color_cycling:
      break;
    case e_game_menu_option::tutorial_hints:
      break;
    case e_game_menu_option::show_fog_of_war:
      ts.map_updater().mutate_options_and_redraw(
          [&]( MapUpdaterOptions& options ) {
            options.render_fog_of_war = false;
          } );
      break;
  }
}

// TODO: temporary until we implement all of the options.
bool is_checkbox_enabled( e_game_menu_option option ) {
  switch( option ) {
    case e_game_menu_option::show_indian_moves:
      return true;
    case e_game_menu_option::show_foreign_moves:
      return false;
    case e_game_menu_option::fast_piece_slide:
      return true;
    case e_game_menu_option::end_of_turn:
      return true;
    case e_game_menu_option::autosave:
      return true;
    case e_game_menu_option::combat_analysis:
      return false;
    case e_game_menu_option::water_color_cycling:
      return true;
    case e_game_menu_option::tutorial_hints:
      return false;
    case e_game_menu_option::show_fog_of_war:
      return true;
  }
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> open_game_options_box( SS& ss, TS& ts ) {
  auto& game_menu_options =
      ss.settings.in_game_options.game_menu_options;
  refl::enum_map<e_game_menu_option, CheckBoxInfo> boxes;
  for( e_game_menu_option option :
       refl::enum_values<e_game_menu_option> )
    boxes[option] = CheckBoxInfo{
      .name = IGui::identifier_to_display_name(
          refl::enum_value_name( option ) ),
      .on       = game_menu_options[option],
      .disabled = !is_checkbox_enabled( option ) };

  co_await ts.gui.enum_check_boxes(
      "Select or unselect game options:", boxes );

  for( auto [option, info] : boxes ) {
    bool const had_previously = game_menu_options[option];
    bool const has_now        = info.on;
    game_menu_options[option] = has_now;
    if( has_now && !had_previously )
      on_option_enabled( ts, option );
    else if( !has_now && had_previously )
      on_option_disabled( ts, option );
  }
}

bool disable_game_option( SS& ss, TS& ts,
                          e_game_menu_option option ) {
  auto& game_menu_options =
      ss.settings.in_game_options.game_menu_options;
  bool const old_value      = game_menu_options[option];
  game_menu_options[option] = false;
  if( game_menu_options[option] != old_value )
    on_option_disabled( ts, option );
  return old_value;
}

/****************************************************************
** Lua
*****************************************************************/
namespace {

LUA_FN( list_flags, lua::table ) {
  lua::table lst = st.table.create();
  int i          = 1;
  for( auto const flag : refl::enum_values<e_game_menu_option> )
    lst[i++] = base::to_str( flag );
  return lst;
};

LUA_FN( get_flag, bool, e_game_menu_option flag ) {
  SS& ss = st["SS"].as<SS&>();
  return ss.settings.in_game_options.game_menu_options[flag];
};

LUA_FN( set_flag, void, e_game_menu_option option, bool value ) {
  SS& ss = st["SS"].as<SS&>();
  TS& ts = st["TS"].as<TS&>();

  ss.settings.in_game_options.game_menu_options[option] = value;
  if( value )
    on_option_enabled( ts, option );
  else
    on_option_disabled( ts, option );
};

} // namespace

} // namespace rn
