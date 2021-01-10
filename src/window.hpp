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

// magic enum
#include "magic_enum.hpp"

// c++ standard library
#include <string_view>
#include <vector>

namespace rn {

struct Plane;
Plane* window_plane();

} // namespace rn

namespace rn::ui {

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
void ok_cancel( std::string_view                   msg,
                std::function<void( e_ok_cancel )> on_result );

void text_input_box(
    std::string_view title, std::string_view msg,
    ValidatorFunc                             validator,
    std::function<void( maybe<std::string> )> on_result );

waitable<maybe<int>> int_input_box( std::string_view title,
                                    std::string_view msg,
                                    maybe<int> min = nothing,
                                    maybe<int> max = nothing );

waitable<maybe<std::string>> str_input_box(
    std::string_view title, std::string_view msg );

/****************************************************************
** Generic Option-Select Window
*****************************************************************/
void select_box(
    std::string_view title, std::vector<std::string> options,
    std::function<void( std::string const& )> on_result );

waitable<std::string> select_box(
    std::string_view title, std::vector<std::string> options );

template<typename Enum>
void select_box_enum( std::string_view            title,
                      std::vector<Enum> const&    options,
                      std::function<void( Enum )> on_result ) {
  // map over member function?
  std::vector<std::string> words;
  for( auto option : options )
    words.push_back(
        std::string( enum_to_display_name( option ) ) );
  select_box(
      title, words,
      [on_result = std::move( on_result ),
       options]( std::string const& result ) {
        for( auto const& option : options ) {
          if( result == enum_to_display_name( option ) ) {
            on_result( option );
            return;
          }
        }
        SHOULD_NOT_BE_HERE;
      } );
}

template<typename Enum>
waitable<Enum> select_box_enum(
    std::string_view title, std::vector<Enum> const& options ) {
  waitable_promise<Enum> s_promise;
  select_box_enum<Enum>( title, options,
                         [s_promise]( Enum result ) mutable {
                           s_promise.set_value( result );
                         } );
  return s_promise.get_future();
}

template<typename Enum>
void select_box_enum( std::string_view            title,
                      std::function<void( Enum )> on_result ) {
  static const std::vector<Enum> options = [] {
    return std::vector<Enum>(
        magic_enum::enum_values<Enum>().begin(),
        magic_enum::enum_values<Enum>().end() );
  }();
  select_box_enum( title, options, std::move( on_result ) );
}

template<typename Enum>
waitable<Enum> select_box_enum( std::string_view title ) {
  waitable_promise<Enum> s_promise;
  select_box_enum<Enum>( title,
                         [s_promise]( Enum result ) mutable {
                           s_promise.set_value( result );
                         } );
  return s_promise.get_future();
}

/****************************************************************
** Canned Option-Select Windows
*****************************************************************/
void yes_no( std::string_view                 title,
             std::function<void( e_confirm )> on_result );

waitable<e_confirm> yes_no( std::string_view title );

/****************************************************************
** Testing Only
*****************************************************************/
void window_test();

} // namespace rn::ui
