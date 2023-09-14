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
wait<maybe<string>> IGui::optional_choice(
    ChoiceConfig const& config ) {
  co_return co_await choice( config, e_input_required::no );
}

wait<string> IGui::required_choice(
    ChoiceConfig const& config ) {
  maybe<string> const res =
      co_await choice( config, e_input_required::yes );
  CHECK( res.has_value() );
  co_return *res;
}

wait<maybe<string>> IGui::optional_string_input(
    StringInputConfig const& config ) {
  co_return co_await string_input( config,
                                   e_input_required::no );
}

wait<string> IGui::required_string_input(
    StringInputConfig const& config ) {
  maybe<string> const res =
      co_await string_input( config, e_input_required::yes );
  CHECK( res.has_value() );
  co_return *res;
}

wait<maybe<int>> IGui::optional_int_input(
    IntInputConfig const& config ) {
  co_return co_await int_input( config, e_input_required::no );
}

wait<int> IGui::required_int_input(
    IntInputConfig const& config ) {
  maybe<int> const res =
      co_await int_input( config, e_input_required::yes );
  CHECK( res.has_value() );
  co_return *res;
}

wait<maybe<ui::e_confirm>> IGui::optional_yes_no(
    YesNoConfig const& config ) {
  vector<ChoiceConfigOption> options{
      { "yes", config.yes_label }, { "no", config.no_label } };
  if( config.no_comes_first )
    reverse( options.begin(), options.end() );
  maybe<string> const str_res = co_await optional_choice(
      { .msg = config.msg, .options = std::move( options ) } );
  if( !str_res.has_value() ) co_return nothing;
  CHECK( *str_res == "no" || *str_res == "yes",
         "invalid value for option choice result: {}",
         *str_res );
  if( *str_res == "no" ) co_return ui::e_confirm::no;
  if( *str_res == "yes" ) co_return ui::e_confirm::yes;
  FATAL( "unexpected input result: {}", *str_res );
}

wait<ui::e_confirm> IGui::required_yes_no(
    YesNoConfig const& config ) {
  vector<ChoiceConfigOption> options{
      { "yes", config.yes_label }, { "no", config.no_label } };
  if( config.no_comes_first )
    reverse( options.begin(), options.end() );
  string const str_res = co_await required_choice(
      { .msg = config.msg, .options = std::move( options ) } );
  CHECK( str_res == "no" || str_res == "yes",
         "invalid value for option choice result: {}", str_res );
  if( str_res == "no" ) co_return ui::e_confirm::no;
  if( str_res == "yes" ) co_return ui::e_confirm::yes;
  FATAL( "unexpected input result: {}", str_res );
}

string IGui::identifier_to_display_name( string_view ident ) {
  return base::capitalize_initials(
      base::str_replace_all( ident, { { "_", " " } } ) );
}

} // namespace rn
