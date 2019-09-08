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
#include "ui-enums.hpp"

// c++ standard library
#include <string_view>
#include <vector>

namespace rn {

struct Plane;
Plane* window_plane();

} // namespace rn

namespace rn::ui {

void message_box( std::string_view msg );

template<typename... Args>
void message_box( std::string_view msg, Args&&... args ) {
  return message_box(
      fmt::format( msg, std::forward<Args>( args )... ) );
}

enum class e_( unit_selection, //
               clear_orders,   //
               activate        // implies clear_orders
);

struct UnitSelection {
  UnitId           id;
  e_unit_selection what;
};

Vec<UnitSelection> unit_selection_box( Vec<UnitId> const& ids_,
                                       bool allow_activation );

/****************************************************************
** Async Windows
*****************************************************************/
namespace async {

void ok_cancel( std::string_view                   msg,
                std::function<void( e_ok_cancel )> on_result );

}

/****************************************************************
** Simple Option-Select Window
*****************************************************************/
std::string select_box( std::string_view title,
                        Vec<Str>         options );

template<typename Enum>
Enum select_box_enum( std::string_view title,
                      Vec<Enum> const& options ) {
  // map over member function?
  std::vector<std::string> words;
  for( auto option : options )
    words.push_back( enum_to_display_name( option ) );
  auto result = select_box( title, words );
  for( auto const& option : options )
    if( result == enum_to_display_name( option ) ) return option;
  SHOULD_NOT_BE_HERE;
}

template<typename Enum>
Enum select_box_enum( std::string_view title ) {
  Vec<Enum> options;
  options.reserve( Enum::_size() );
  for( auto val : values<Enum> ) options.push_back( val );
  return select_box_enum( title, options );
}

/****************************************************************
** Canned Option-Select Windows
*****************************************************************/
e_confirm yes_no( std::string_view title );

/****************************************************************
** Testing Only
*****************************************************************/
void window_test();

} // namespace rn::ui
