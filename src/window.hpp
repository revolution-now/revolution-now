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
#include "id.hpp"
#include "ui-enums.hpp"
#include "waitable.hpp"

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
waitable<> message_box( std::string_view msg );

template<typename... Args>
waitable<> message_box( std::string_view msg, Args&&... args ) {
  return message_box(
      fmt::format( msg, std::forward<Args>( args )... ) );
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

waitable<std::vector<UnitSelection>> unit_selection_box(
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
waitable<e_ok_cancel> ok_cancel( std::string_view msg );

template<typename... Args>
waitable<e_ok_cancel> ok_cancel( std::string_view question,
                                 Args&&... args ) //
    requires( sizeof...( Args ) > 0 ) {
  return ok_cancel(
      fmt::format( question, std::forward<Args>( args )... ) );
}

void text_input_box(
    std::string_view title, std::string_view msg,
    ValidatorFunc                             validator,
    std::function<void( maybe<std::string> )> on_result );

waitable<maybe<int>> int_input_box( std::string_view title,
                                    std::string_view msg,
                                    maybe<int> min = nothing,
                                    maybe<int> max = nothing );

waitable<maybe<std::string>> str_input_box(
    std::string_view title, std::string_view msg,
    std::string_view initial_text );

/****************************************************************
** Generic Option-Select Window
*****************************************************************/
waitable<std::string> select_box(
    std::string_view title, std::vector<std::string> options );

// FIXME: clang can't seem to handle function template corouti-
// nes, without emitting warnings, so to work around that we make
// this a niebloid.
template<typename Enum>
struct SelectBoxEnum {
  waitable<Enum> operator()(
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

  waitable<Enum> operator()( std::string_view title ) const {
    static const std::vector<Enum> options = [] {
      return std::vector<Enum>(
          enum_traits<Enum>::values.begin(),
          enum_traits<Enum>::values.end() );
    }();
    return (*this)( title, options );
  }
};

template<typename Enum>
inline constexpr SelectBoxEnum<Enum> select_box_enum{};

/****************************************************************
** Canned Option-Select Windows
*****************************************************************/
waitable<e_confirm> yes_no( std::string_view title );

template<typename... Args>
waitable<e_confirm> yes_no( std::string_view question,
                            Args&&... args ) {
  return yes_no(
      fmt::format( question, std::forward<Args>( args )... ) );
}

/****************************************************************
** Testing Only
*****************************************************************/
void window_test();

} // namespace rn::ui
