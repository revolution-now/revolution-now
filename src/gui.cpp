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

wait<string> RealGui::choice( ChoiceConfig const& config ) {
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
  vector<string> options;
  for( ChoiceConfigOption option : config.options )
    options.push_back( option.display_name );
  if( config.initial_selection.has_value() ) {
    CHECK_GE( *config.initial_selection, 0 );
    CHECK_LT( *config.initial_selection, int( options.size() ) );
  }
  int selected = co_await window_plane_.select_box(
      config.msg, options,
      config.initial_selection.value_or( 0 ) );
  co_return config.options[selected].key;
}

wait<string> RealGui::string_input(
    StringInputConfig const& config ) {
  maybe<string> res;
  // FIXME: need to use a different function here that just re-
  // quires input.
  while( !res.has_value() )
    res = co_await window_plane_.str_input_box(
        "title?", config.msg, config.initial_text );
  DCHECK( res.has_value() );
  co_return *res;
}

wait<int> RealGui::int_input( IntInputConfig const& config ) {
  maybe<int> res;
  // FIXME: need to use a different function here that just re-
  // quires input.
  while( !res.has_value() )
    res = co_await window_plane_.int_input_box( {
        .title   = "title?",
        .msg     = config.msg,
        .min     = config.min,
        .max     = config.max,
        .initial = config.initial_value,
    } );
  DCHECK( res.has_value() );
  co_return *res;
}

wait<chrono::microseconds> RealGui::wait_for(
    chrono::microseconds time ) {
  chrono::microseconds actual = co_await time;
  co_return actual;
}

} // namespace rn
