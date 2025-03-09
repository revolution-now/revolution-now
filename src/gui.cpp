/****************************************************************
**gui.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-24.
*
* Description: IGui implementation for creating a real GUI.
*
*****************************************************************/
#include "gui.hpp"

// Revolution Now
#include "co-time.hpp"
#include "co-wait.hpp"
#include "plane-stack.hpp"
#include "views.hpp"
#include "window.hpp"
#include "woodcut.hpp"

// C++ standard library
#include <algorithm>
#include <unordered_set>
#include <vector>

using namespace std;

namespace rn {

/****************************************************************
** RealGui
*****************************************************************/
WindowPlane& RealGui::window_plane() const {
  return planes_.get().window;
}

wait<> RealGui::message_box( string const& msg ) {
  return window_plane().message_box( msg );
}

void RealGui::transient_message_box( string const& msg ) {
  window_plane().transient_message_box( msg );
}

wait<chrono::microseconds> RealGui::wait_for(
    chrono::microseconds time ) {
  chrono::microseconds actual = co_await time;
  co_return actual;
}

wait<maybe<string>> RealGui::choice(
    ChoiceConfig const& config ) {
  bool const required =
      !config.cancel_actions.disallow_escape_key ||
      !config.cancel_actions.disallow_clicking_outside;
  if( required ) {
    // If the input is required then there must be at least one
    // enabled choice.
    bool has_enabled = false;
    for( auto& option : config.options ) {
      bool const enabled = !option.disabled;
      has_enabled |= enabled;
    }
    CHECK( has_enabled,
           "The game attempted to open a select box with input "
           "required but with no enabled items." );
  }
  if( config.sort ) {
    ChoiceConfig new_config = config;
    std::sort( new_config.options.begin(),
               new_config.options.end(), []( auto& l, auto& r ) {
                 return l.display_name < r.display_name;
               } );
    // Recurse but this time with no sorting.
    new_config.sort = false;
    co_return co_await choice( new_config );
  }
  {
    // Sanity check.
    unordered_set<string> seen_key;
    unordered_set<string> seen_display;
    for( ChoiceConfigOption option : config.options ) {
      DCHECK( !seen_key.contains( option.key ),
              "key {} appears twice.", option.key );
      // Note that we don't do a similar uniqueness check for the
      // display name because those are allowed to conincide in
      // some cases, e.g. when selecting from the immigrant pool.
      seen_key.insert( option.key );
      seen_display.insert( option.display_name );
    }
  }
  vector<SelectBoxOption> options;
  options.reserve( config.options.size() );
  for( ChoiceConfigOption option : config.options )
    options.push_back( { .name    = option.display_name,
                         .enabled = !option.disabled } );
  if( config.initial_selection.has_value() ) {
    CHECK_GE( *config.initial_selection, 0 );
    CHECK_LT( *config.initial_selection, int( options.size() ) );
  }
  maybe<int> const selected = co_await window_plane().select_box(
      config.msg, options, config.cancel_actions,
      config.initial_selection );
  if( !selected.has_value() ) {
    // User cancelled.
    co_return nothing;
  }
  co_return config.options[*selected].key;
}

wait<maybe<string>> RealGui::string_input(
    StringInputConfig const& config ) {
  maybe<string> const res =
      co_await window_plane().str_input_box(
          config.msg, config.cancel_actions,
          config.initial_text );
  if( !res.has_value() ) {
    // User cancelled.
    co_return nothing;
  }
  co_return *res;
}

wait<maybe<int>> RealGui::int_input(
    IntInputConfig const& config ) {
  maybe<int> const res = co_await window_plane().int_input_box( {
    .msg            = config.msg,
    .min            = config.min,
    .max            = config.max,
    .cancel_actions = config.cancel_actions,
    .initial        = config.initial_value,
  } );
  if( !res.has_value() ) {
    // User cancelled.
    co_return nothing;
  }
  co_return *res;
}

wait<> RealGui::display_woodcut( e_woodcut cut ) {
  co_await internal::show_woodcut( *this, cut );
}

int RealGui::total_windows_created() const {
  return window_plane().num_windows_created();
}

wait<unordered_map<int, bool>> RealGui::check_box_selector(
    string const& title,
    unordered_map<int, CheckBoxInfo> const& items ) {
  using namespace ui;
  // Add check boxes.
  auto boxes_array = make_unique<VerticalArrayView>(
      VerticalArrayView::align::left );

  unordered_map<int, LabeledCheckBoxView const*> boxes;
  for( int item = 0; item < int( items.size() ); ++item ) {
    auto iter = items.find( item );
    if( iter == items.end() ) continue;
    CheckBoxInfo const& info = iter->second;
    auto labeled_box = make_unique<TextLabeledCheckBoxView>(
        textometer_, string( info.name ), info.on );
    if( info.disabled ) labeled_box->set_disabled( true );
    boxes[item] = labeled_box.get();
    boxes_array->add_view( std::move( labeled_box ) );
  }
  boxes_array->recompute_child_positions();

  unordered_map<int, bool> res;
  for( auto& [item, info] : items ) res[item] = info.on;

  auto on_ok = [&] {
    for( auto [item, box] : boxes ) res[item] = box->on();
  };

  auto const _ = co_await ok_cancel_box(
      title, std::move( boxes_array ), on_ok );
  // !! At this point the view and all pointers into it above
  // have been destroyed.

  co_return res;
}

wait<> RealGui::ok_cancel_box(
    string const& title, unique_ptr<ui::View> view,
    base::function_ref<void()> const on_ok ) {
  using namespace ui;
  auto top = make_unique<VerticalArrayView>(
      VerticalArrayView::align::center );
  // Add text.
  auto text_view = make_unique<TextView>( textometer_, title );
  top->add_view( std::move( text_view ) );
  // Add some space between title and main view.
  top->add_view(
      make_unique<EmptyView>( Delta{ .w = 1, .h = 2 } ) );
  top->add_view( std::move( view ) );
  // Add some space between boxes and buttons.
  top->add_view(
      make_unique<EmptyView>( Delta{ .w = 1, .h = 4 } ) );

  // Add buttons.
  auto buttons_view = make_unique<OkCancelView2>( textometer_ );
  OkCancelView2* buttons = buttons_view.get();
  top->add_view( std::move( buttons_view ) );

  // Finalize top-level array.
  top->recompute_child_positions();

  // Create window.
  WindowManager& wm = window_plane().manager();
  Window window( wm );
  window.set_view( std::move( top ) );
  window.autopad_me();
  // Must be done after auto-padding.
  window.center_me();

  // The window (and view that it contains) must be kept alive
  // while we call the on_ok method, which typically needs to ex-
  // tract info from the view.
  if( co_await buttons->next() == e_ok_cancel::ok ) on_ok();
}

} // namespace rn
