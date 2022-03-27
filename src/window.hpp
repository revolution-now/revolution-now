/****************************************************************
**window.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-30.
*
* Description: Handles windowing system for user interaction.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "enum.hpp"
#include "error.hpp"
#include "expect.hpp"
#include "ui-enums.hpp"
#include "unit-id.hpp"
#include "wait.hpp"

// refl
#include "refl/query-enum.hpp"

// c++ standard library
#include <string_view>
#include <vector>

namespace rn {

struct Plane;
Plane* window_plane();

} // namespace rn

namespace rn::ui {

// Pops up a box that displays a message to the user but takes no
// user input apart from waiting for the <CR> or Space keys to be
// pressed, which then closes the window. It takes markup text as
// input and it will reflow the message.
wait<> message_box_basic( std::string_view msg );

template<typename... Args>
wait<> message_box( std::string_view msg, Args&&... args ) {
  return message_box_basic( fmt::format(
      fmt::runtime( msg ), std::forward<Args>( args )... ) );
}

enum class e_unit_selection {
  clear_orders,
  activate // implies clear_orders
};

struct UnitSelection {
  UnitId           id;
  e_unit_selection what;
};
NOTHROW_MOVE( UnitSelection );

wait<std::vector<UnitSelection>> unit_selection_box(
    std::vector<UnitId> const& ids_, bool allow_activation );

/****************************************************************
** Validators
*****************************************************************/
// These should probably be moved elsewhere.

using ValidatorFunc = std::function<bool( std::string const& )>;

// Makes a validator that enforces that the input be parsable
// into an integer and that (optionally) it is within [min, max].
ValidatorFunc make_int_validator( maybe<int> min,
                                  maybe<int> max );

/****************************************************************
** Windows
*****************************************************************/
wait<e_ok_cancel> ok_cancel( std::string_view msg );

template<base::Show... Args>
wait<e_ok_cancel> ok_cancel( std::string_view fmt_str,
                             Args&&... args ) {
  return ok_cancel( fmt::format(
      fmt::runtime( fmt_str ), std::forward<Args>( args )... ) );
}

struct IntInputBoxOptions {
  std::string_view title   = "";
  std::string_view msg     = "";
  maybe<int>       min     = nothing;
  maybe<int>       max     = nothing;
  maybe<int>       initial = nothing;
};

wait<maybe<int>> int_input_box(
    IntInputBoxOptions const& options );

wait<maybe<std::string>> str_input_box(
    std::string_view title, std::string_view msg,
    std::string_view initial_text );

/****************************************************************
** Generic Option-Select Window
*****************************************************************/
wait<std::string> select_box( std::string_view         title,
                              std::vector<std::string> options );

// FIXME: clang can't seem to handle function template corouti-
// nes, without emitting warnings, so to work around that we make
// this a niebloid.
template<typename Enum>
struct SelectBoxEnum {
  wait<Enum> operator()(
      std::string_view         title,
      std::vector<Enum> const& options ) const {
    // map over member function?
    std::vector<std::string> words;
    for( auto option : options )
      words.push_back(
          std::string( enum_to_display_name( option ) ) );
    std::string str_result = co_await select_box( title, words );
    for( auto const& option : options )
      if( str_result == enum_to_display_name( option ) )
        co_return option;
    SHOULD_NOT_BE_HERE;
  }

  wait<Enum> operator()( std::string_view title ) const {
    static const std::vector<Enum> options = [] {
      return std::vector<Enum>( refl::enum_values<Enum>.begin(),
                                refl::enum_values<Enum>.end() );
    }();
    return ( *this )( title, options );
  }
};

template<typename Enum>
inline constexpr SelectBoxEnum<Enum> select_box_enum{};

/****************************************************************
** Canned Option-Select Windows
*****************************************************************/
wait<e_confirm> yes_no( std::string_view title );

template<typename... Args>
wait<e_confirm> yes_no( std::string_view question,
                        Args&&... args ) {
  return yes_no( fmt::format( fmt::runtime( question ),
                              std::forward<Args>( args )... ) );
}

/****************************************************************
** Testing Only
*****************************************************************/
void window_test();

} // namespace rn::ui
