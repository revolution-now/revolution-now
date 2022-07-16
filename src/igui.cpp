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

// base
#include "base/string.hpp"

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
  if( res == "no" ) co_return ui::e_confirm::no;
  if( res == "yes" ) co_return ui::e_confirm::yes;
  FATAL( "unexpected input result: {}", res );
}

string IGui::identifier_to_display_name(
    string_view ident ) const {
  return base::capitalize_initials(
      base::str_replace_all( ident, { { "_", " " } } ) );
}

} // namespace rn
