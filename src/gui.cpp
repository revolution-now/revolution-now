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
#include "co-wait.hpp"
#include "window.hpp"

// C++ standard library
#include <unordered_set>
#include <vector>

using namespace std;

namespace rn {

/****************************************************************
** RealGui
*****************************************************************/
wait<> RealGui::message_box( std::string_view msg ) {
  return ui::message_box( msg );
}

wait<std::string> RealGui::choice( ChoiceConfig const& config ) {
  {
    // Sanity check.
    unordered_set<string> seen_key;
    unordered_set<string> seen_display;
    for( ChoiceConfig::Option option : config.options ) {
      DCHECK( !seen_key.contains( option.key ) );
      DCHECK( !seen_display.contains( option.display_name ) );
      seen_key.insert( option.key );
      seen_display.insert( option.display_name );
    }
  }
  vector<string> options;
  for( ChoiceConfig::Option option : config.options )
    options.push_back( option.display_name );
  int selected = co_await ui::select_box( config.msg, options );
  co_return config.options[selected].key;
}

} // namespace rn
