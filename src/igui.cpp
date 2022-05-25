/****************************************************************
**igui.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-25.
*
* Description: Injectable interface for in-game GUI interactions.
*
*****************************************************************/
#include "igui.hpp"

// Revolution Now
#include "co-wait.hpp"

using namespace std;

namespace rn {

/****************************************************************
** IGui
*****************************************************************/
wait<ui::e_confirm> IGui::yes_no( YesNoConfig const& config ) {
  vector<ChoiceConfigOption> options{
      { "yes", config.yes_label }, { "no", config.no_label } };
  if( config.no_comes_first )
    reverse( options.begin(), options.end() );
  string res = co_await choice(
      { .msg = config.msg, .options = std::move( options ) } );
  DCHECK( res == "no" || res == "yes" );
  co_return( res == "no" ) ? ui::e_confirm::no
                           : ui::e_confirm::yes;
}

} // namespace rn
