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
#include "aliases.hpp"
#include "enum.hpp"
#include "errors.hpp"
#include "id.hpp"
#include "sync-future.hpp"
#include "ui-enums.hpp"

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

sync_future<> message_box( std::string_view msg );

template<typename... Args>
sync_future<> message_box( std::string_view msg,
                           Args&&... args ) {
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

sync_future<Vec<UnitSelection>> unit_selection_box(
    Vec<UnitId> const& ids_, bool allow_activation );

/****************************************************************
** Validators
*****************************************************************/
// These should probably be moved elsewhere.

using ValidatorFunc = std::function<bool( std::string const& )>;

// Makes a validator that enforces that the input be parsable
// into an integer and that (optionally) it is within [min, max].
ValidatorFunc make_int_validator( Opt<int> min, Opt<int> max );

/****************************************************************
** Windows
*****************************************************************/
void ok_cancel( std::string_view                   msg,
                std::function<void( e_ok_cancel )> on_result );

void text_input_box(
    std::string_view title, std::string_view msg,
    ValidatorFunc                           validator,
    std::function<void( Opt<std::string> )> on_result );

sync_future<Opt<int>> int_input_box( std::string_view title,
                                     std::string_view msg,
                                     Opt<int> min = nothing,
                                     Opt<int> max = nothing );

sync_future<Opt<std::string>> str_input_box(
    std::string_view title, std::string_view msg );

/****************************************************************
** Generic Option-Select Window
*****************************************************************/
void select_box(
    std::string_view title, Vec<Str> options,
    std::function<void( std::string const& )> on_result );

sync_future<std::string> select_box( std::string_view title,
                                     Vec<Str>         options );

template<typename Enum>
void select_box_enum( std::string_view            title,
                      Vec<Enum> const&            options,
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
sync_future<Enum> select_box_enum( std::string_view title,
                                   Vec<Enum> const& options ) {
  sync_promise<Enum> s_promise;
  select_box_enum<Enum>( title, options,
                         [s_promise]( Enum result ) mutable {
                           s_promise.set_value( result );
                         } );
  return s_promise.get_future();
}

template<typename Enum>
void select_box_enum( std::string_view            title,
                      std::function<void( Enum )> on_result ) {
  static const Vec<Enum> options = [] {
    return Vec<Enum>( magic_enum::enum_values<Enum>().begin(),
                      magic_enum::enum_values<Enum>().end() );
  }();
  select_box_enum( title, options, std::move( on_result ) );
}

template<typename Enum>
sync_future<Enum> select_box_enum( std::string_view title ) {
  sync_promise<Enum> s_promise;
  select_box_enum<Enum>( title,
                         [s_promise]( Enum result ) mutable {
                           s_promise.set_value( result );
                         } );
  return s_promise.get_future();
}

/****************************************************************
** Combinators
*****************************************************************/
// Repeatedly calls `to_repeat` to get a future. Each time the
// future yields a value it will validate it using the
// `get_error` function. If there is an error it will display it
// and then keep calling `to_repeat` until a valid result ob-
// tains.
template<typename Ret>
sync_future<Ret> repeat_until(
    std::function<sync_future<Ret>()> to_repeat,
    std::function<valid_or<std::string>( Ret const& )>
        get_error ) {
  return to_repeat() >> [=]( Ret const& val ) {
    if( auto xp = get_error( val ); !xp )
      return message_box( xp.error() ).next( [=] {
        // Loop by recursion.
        return repeat_until<Ret>( to_repeat, get_error );
      } );
    return make_sync_future<Ret>( val );
  };
}

// Similar to above, but the function being repeated can return a
// nothing, basically meaning that the operation is cancelled.
template<typename Ret>
sync_future<Opt<Ret>> repeat_until_or_cancel(
    std::function<sync_future<Opt<Ret>>()> to_repeat,
    std::function<valid_or<std::string>( Ret const& )>
        get_error ) {
  return to_repeat() >> [=]( Opt<Ret> const& maybe_val ) {
    if( !maybe_val.has_value() )
      return make_sync_future<Opt<Ret>>( nothing );
    if( auto xp = get_error( *maybe_val ); !xp )
      return message_box( xp.error() ).next( [=] {
        // Loop by recursion.
        return repeat_until_or_cancel<Ret>( to_repeat,
                                            get_error );
      } );
    return make_sync_future<Opt<Ret>>( *maybe_val );
  };
}

/****************************************************************
** Canned Option-Select Windows
*****************************************************************/
void yes_no( std::string_view                 title,
             std::function<void( e_confirm )> on_result );

sync_future<e_confirm> yes_no( std::string_view title );

/****************************************************************
** Testing Only
*****************************************************************/
void window_test();

} // namespace rn::ui
