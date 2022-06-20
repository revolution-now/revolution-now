/****************************************************************
**igui.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-24.
*
* Description: Injectable interface for in-game GUI interactions.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "maybe.hpp"
#include "wait.hpp"

// Rds
#include "igui.rds.hpp"

// refl
#include "refl/enum-map.hpp"
#include "refl/ext.hpp"
#include "refl/query-enum.hpp"

// base
#include "base/fmt.hpp"
#include "base/no-default.hpp"

// C++ standard library
#include <chrono>
#include <string>
#include <string_view>
#include <vector>

namespace rn {

/****************************************************************
** IGui
*****************************************************************/
// This class provides a mockable interface containing methods
// for each of the GUI-related things that game logic might have
// to do, which is needed for testability of that code. Most of
// these methods will naturally have `wait<T>` return values.
//
// This should be threaded down through any function that needs
// to interact with the player through the gui.
//
struct IGui {
  virtual ~IGui() = default;

  // Displays a message box. Hitting basically any key or
  // clicking the mouse (anywhere) should close it.
  virtual wait<> message_box( std::string_view msg ) = 0;

  // Display a message and one or more choices and let the user
  // choose one.
  virtual wait<std::string> choice(
      ChoiceConfig const& config ) = 0;

  // Display a prompt and ask (require) a string input from the
  // user.
  virtual wait<std::string> string_input(
      StringInputConfig const& config ) = 0;

  // Display a prompt and ask (require) an integer input from the
  // user.
  virtual wait<int> int_input(
      IntInputConfig const& config ) = 0;

  // Waits for the given amount of time and then returns the
  // amount of time actually waited.
  virtual wait<std::chrono::microseconds> wait_for(
      std::chrono::microseconds time ) = 0;

  // For convenience.  Should not be overridden.
  template<typename Arg, typename... Rest>
  wait<> message_box( std::string_view msg, Arg&& arg,
                      Rest&&... rest ) {
    return message_box( fmt::format(
        fmt::runtime( msg ), std::forward<Arg>( arg ),
        std::forward<Rest>( rest )... ) );
  }

  // For convenience. Should not be overridden.
  wait<ui::e_confirm> yes_no( YesNoConfig const& config );

  template<refl::ReflectedEnum E>
  wait<maybe<E>> enum_choice(
      EnumChoiceConfig const&               enum_config,
      refl::enum_map<E, std::string> const& names );

  // This one just uses the names of the enums as display names.
  // This is mostly for debug related UIs.
  template<refl::ReflectedEnum E>
  wait<maybe<E>> enum_choice(
      EnumChoiceConfig const& enum_config );

  // Even more minimal for quick and dirty menus, just asks "Se-
  // lect One" as the message and allows escaping. If sort ==
  // true then the items will be sorted by display name.
  template<refl::ReflectedEnum E>
  wait<maybe<E>> enum_choice( bool sort = false );

  // For convenience while developing, shouldn't really be used
  // to provide proper names for things for the player.
  std::string identifier_to_display_name(
      std::string_view ident ) const;
};

template<refl::ReflectedEnum E>
wait<maybe<E>> IGui::enum_choice(
    EnumChoiceConfig const&               enum_config,
    refl::enum_map<E, std::string> const& names ) {
  ChoiceConfig config{ .msg  = enum_config.msg,
                       .sort = enum_config.sort };
  if( !enum_config.choice_required ) config.key_on_escape = "-";
  for( E item : refl::enum_values<E> )
    config.options.push_back( ChoiceConfigOption{
        .key = std::string( refl::enum_value_name( item ) ),
        .display_name = names[item] } );
  std::string res = co_await choice( config );
  // If hitting escape was allowed, and if that happened, then
  // this should do the right thing and return nothing.
  co_return refl::enum_from_string<E>( res );
}

template<refl::ReflectedEnum E>
wait<maybe<E>> IGui::enum_choice(
    EnumChoiceConfig const& enum_config ) {
  refl::enum_map<E, std::string> names;
  for( E item : refl::enum_values<E> )
    names[item] = identifier_to_display_name(
        refl::enum_value_name( item ) );
  co_return co_await enum_choice( enum_config, names );
}

template<refl::ReflectedEnum E>
wait<maybe<E>> IGui::enum_choice( bool sort ) {
  EnumChoiceConfig config{ .msg             = "Select One",
                           .choice_required = false,
                           .sort            = sort };
  co_return co_await enum_choice<E>( config );
}

} // namespace rn
