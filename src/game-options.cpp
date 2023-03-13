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

using namespace std;

namespace rn {

namespace {

void on_option_enabled( TS& ts, e_game_flag_option option ) {
  switch( option ) {
    case e_game_flag_option::show_indian_moves:
      break;
    case e_game_flag_option::show_foreign_moves:
      break;
    case e_game_flag_option::fast_piece_slide:
      break;
    case e_game_flag_option::end_of_turn:
      break;
    case e_game_flag_option::autosave:
      break;
    case e_game_flag_option::combat_analysis:
      break;
    case e_game_flag_option::water_color_cycling:
      break;
    case e_game_flag_option::tutorial_hints:
      break;
    case e_game_flag_option::show_fog_of_war:
      ts.map_updater.mutate_options_and_redraw(
          [&]( MapUpdaterOptions& options ) {
            options.render_fog_of_war = true;
          } );
      break;
  }
}

void on_option_disabled( TS& ts, e_game_flag_option option ) {
  switch( option ) {
    case e_game_flag_option::show_indian_moves:
      break;
    case e_game_flag_option::show_foreign_moves:
      break;
    case e_game_flag_option::fast_piece_slide:
      break;
    case e_game_flag_option::end_of_turn:
      break;
    case e_game_flag_option::autosave:
      break;
    case e_game_flag_option::combat_analysis:
      break;
    case e_game_flag_option::water_color_cycling:
      break;
    case e_game_flag_option::tutorial_hints:
      break;
    case e_game_flag_option::show_fog_of_war:
      ts.map_updater.mutate_options_and_redraw(
          [&]( MapUpdaterOptions& options ) {
            options.render_fog_of_war = false;
          } );
      break;
  }
}

// TODO: temporary until we implement all of the options.
bool is_checkbox_enabled( e_game_flag_option option ) {
  switch( option ) {
    case e_game_flag_option::show_indian_moves:
      return false;
    case e_game_flag_option::show_foreign_moves:
      return false;
    case e_game_flag_option::fast_piece_slide:
      return true;
    case e_game_flag_option::end_of_turn:
      return false;
    case e_game_flag_option::autosave:
      return false;
    case e_game_flag_option::combat_analysis:
      return false;
    case e_game_flag_option::water_color_cycling:
      return false;
    case e_game_flag_option::tutorial_hints:
      return false;
    case e_game_flag_option::show_fog_of_war:
      return true;
  }
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> open_game_options_box( SS& ss, TS& ts ) {
  using namespace ui;

  auto& flags = ss.settings.game_options.flags;

  auto top_array = make_unique<VerticalArrayView>(
      VerticalArrayView::align::center );

  // Add text.
  auto text_view = make_unique<TextView>(
      "Select or unselect game options:" );
  top_array->add_view( std::move( text_view ) );
  // Add some space between title and check boxes.
  top_array->add_view(
      make_unique<EmptyView>( Delta{ .w = 1, .h = 2 } ) );

  // Add check boxes.
  auto boxes_array = make_unique<VerticalArrayView>(
      VerticalArrayView::align::left );

  refl::enum_map<e_game_flag_option, LabeledCheckBoxView const*>
      boxes;
  for( e_game_flag_option option :
       refl::enum_values<e_game_flag_option> ) {
    auto labeled_box = make_unique<LabeledCheckBoxView>(
        IGui::identifier_to_display_name(
            refl::enum_value_name( option ) ),
        flags[option] );
    if( !is_checkbox_enabled( option ) )
      labeled_box->set_disabled( true );
    boxes[option] = labeled_box.get();
    boxes_array->add_view( std::move( labeled_box ) );
  }
  boxes_array->recompute_child_positions();

  top_array->add_view( std::move( boxes_array ) );
  // Add some space between boxes and buttons.
  top_array->add_view(
      make_unique<EmptyView>( Delta{ .w = 1, .h = 4 } ) );

  // Add buttons.
  // FIXME: get rid of the buttons here; the player should just
  // be able to set the check boxes and then close the window.
  auto buttons_view          = make_unique<ui::OkCancelView2>();
  ui::OkCancelView2* buttons = buttons_view.get();
  top_array->add_view( std::move( buttons_view ) );

  // Finalize top-level array.
  top_array->recompute_child_positions();

  // Create window.
  WindowManager& wm = ts.planes.window().manager();
  Window         window( wm );
  window.set_view( std::move( top_array ) );
  window.autopad_me();
  // Must be done after auto-padding.
  window.center_me();

  ui::e_ok_cancel const finished = co_await buttons->next();
  if( finished == ui::e_ok_cancel::cancel ) co_return;

  for( auto [option, box] : boxes ) {
    bool const had_previously = flags[option];
    bool const has_now        = box->on();
    flags[option]             = has_now;
    if( has_now && !had_previously )
      on_option_enabled( ts, option );
    else if( !has_now && had_previously )
      on_option_disabled( ts, option );
  }
}

} // namespace rn
