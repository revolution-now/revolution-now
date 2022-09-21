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
#include "window.hpp"

// C++ standard library
#include <algorithm>
#include <unordered_set>
#include <vector>

using namespace std;

namespace rn {

/****************************************************************
** RealGui
*****************************************************************/
wait<> RealGui::message_box( string_view msg ) {
  return window_plane_.message_box( msg );
}

wait<chrono::microseconds> RealGui::wait_for(
    chrono::microseconds time ) {
  chrono::microseconds actual = co_await time;
  co_return actual;
}

wait<maybe<string>> RealGui::choice(
    ChoiceConfig const& config, e_input_required required ) {
  if( required == e_input_required::yes ) {
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
    co_return co_await choice( new_config, required );
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
  maybe<int> const selected = co_await window_plane_.select_box(
      config.msg, options, required,
      config.initial_selection.value_or( 0 ) );
  if( !selected.has_value() ) {
    // User cancelled.
    CHECK( required == e_input_required::no );
    co_return nothing;
  }
  co_return config.options[*selected].key;
}

wait<maybe<string>> RealGui::string_input(
    StringInputConfig const& config,
    e_input_required         required ) {
  maybe<string> const res = co_await window_plane_.str_input_box(
      config.msg, config.initial_text, required );
  if( !res.has_value() ) {
    // User cancelled.
    CHECK( required == e_input_required::no );
    co_return nothing;
  }
  co_return *res;
}

wait<maybe<int>> RealGui::int_input(
    IntInputConfig const& config, e_input_required required ) {
  maybe<int> const res = co_await window_plane_.int_input_box( {
      .msg      = config.msg,
      .min      = config.min,
      .max      = config.max,
      .initial  = config.initial_value,
      .required = required,
  } );
  if( !res.has_value() ) {
    // User cancelled.
    CHECK( required == e_input_required::no );
    co_return nothing;
  }
  co_return *res;
}

} // namespace rn
