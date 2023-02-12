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

enum class e_woodcut;

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

  /* ============================================================
  ** One-way (non-input) GUI/IO stuff.
  ** ===========================================================*/
  // Displays a message box. Hitting basically any key or
  // clicking the mouse (anywhere) should close it.
  virtual wait<> message_box( std::string const& msg ) = 0;

  // For convenience.  Should not be overridden.
  template<typename Arg, typename... Rest>
  wait<> message_box( std::string_view msg, Arg&& arg,
                      Rest&&... rest ) {
    return message_box( fmt::format(
        fmt::runtime( msg ), std::forward<Arg>( arg ),
        std::forward<Rest>( rest )... ) );
  }

  // Waits for the given amount of time and then returns the
  // amount of time actually waited.
  virtual wait<std::chrono::microseconds> wait_for(
      std::chrono::microseconds time ) = 0;

  /* ============================================================
  ** User Input GUI
  ** ===========================================================*/

 public:
  wait<maybe<std::string>> optional_choice(
      ChoiceConfig const& config );

  wait<std::string> required_choice(
      ChoiceConfig const& config );

  wait<maybe<std::string>> optional_string_input(
      StringInputConfig const& config );

  wait<std::string> required_string_input(
      StringInputConfig const& config );

  wait<maybe<int>> optional_int_input(
      IntInputConfig const& config );

  wait<int> required_int_input( IntInputConfig const& config );

  wait<maybe<ui::e_confirm>> optional_yes_no(
      YesNoConfig const& config );

  wait<ui::e_confirm> required_yes_no(
      YesNoConfig const& config );

  template<refl::ReflectedEnum E>
  wait<maybe<E>> optional_enum_choice(
      EnumChoiceConfig const&               enum_config,
      refl::enum_map<E, std::string> const& names,
      refl::enum_map<E, bool> const&        disabled );

  template<refl::ReflectedEnum E>
  wait<E> required_enum_choice(
      EnumChoiceConfig const&               enum_config,
      refl::enum_map<E, std::string> const& names,
      refl::enum_map<E, bool> const&        disabled );

  // All items enabled.
  template<refl::ReflectedEnum E>
  wait<maybe<E>> optional_enum_choice(
      EnumChoiceConfig const&               enum_config,
      refl::enum_map<E, std::string> const& names );

  // All items enabled.
  template<refl::ReflectedEnum E>
  wait<E> required_enum_choice(
      EnumChoiceConfig const&               enum_config,
      refl::enum_map<E, std::string> const& names );

  // This one just uses the names of the enums as display names.
  // This is mostly for debug related UIs.
  template<refl::ReflectedEnum E>
  wait<maybe<E>> optional_enum_choice(
      EnumChoiceConfig const&        enum_config,
      refl::enum_map<E, bool> const& disabled );

  template<refl::ReflectedEnum E>
  wait<E> required_enum_choice(
      EnumChoiceConfig const&        enum_config,
      refl::enum_map<E, bool> const& disabled );

  template<refl::ReflectedEnum E>
  wait<maybe<E>> optional_enum_choice(
      EnumChoiceConfig const& enum_config );

  template<refl::ReflectedEnum E>
  wait<E> required_enum_choice(
      EnumChoiceConfig const& enum_config );

  // Even more minimal for quick and dirty menus, just asks "Se-
  // lect One" as the message and allows escaping. If sort ==
  // true then the items will be sorted by display name.
  template<refl::ReflectedEnum E>
  wait<maybe<E>> optional_enum_choice(
      refl::enum_map<E, bool> const& disabled,
      bool                           sort = false );

  template<refl::ReflectedEnum E>
  wait<E> required_enum_choice(
      refl::enum_map<E, bool> const& disabled,
      bool                           sort = false );

  template<refl::ReflectedEnum E>
  wait<maybe<E>> optional_enum_choice( bool sort = false );

  template<refl::ReflectedEnum E>
  wait<E> required_enum_choice( bool sort = false );

  // For when we want to limit to a subset of the possible enum
  // values.
  template<refl::ReflectedEnum E>
  wait<maybe<E>> partial_optional_enum_choice(
      std::string const& msg, std::vector<E> const& options,
      bool sort = false );

  template<refl::ReflectedEnum E>
  wait<maybe<E>> partial_optional_enum_choice(
      std::vector<E> const& options, bool sort = false );

 protected:
  // Do not call these directly, instead call the ones in the
  // next section that make it explicit in the name and return
  // type as to whether the user input is required or not (or if
  // the user can just hit escape to opt out of the input).

  // Display a message and one or more choices and let the user
  // choose one. This returns a maybe because it is used to im-
  // plement both the optional and required variants.
  virtual wait<maybe<std::string>> choice(
      ChoiceConfig const& config,
      e_input_required    required ) = 0;

  // Display a prompt and ask the user for a string input. This
  // returns a maybe because it is used to implement both the op-
  // tional and required variants.
  virtual wait<maybe<std::string>> string_input(
      StringInputConfig const& config,
      e_input_required         required ) = 0;

  // Display a prompt and ask the user for an integer input. This
  // returns a maybe because it is used to implement both the op-
  // tional and required variants.
  virtual wait<maybe<int>> int_input(
      IntInputConfig const& config,
      e_input_required      required ) = 0;

  /* ============================================================
  ** Woodcuts
  ** ===========================================================*/

 public:
  // Note that this one should not be called directly by normal
  // game code. Instead call the one in the woodcut module that
  // will check to make sure that each woodcut is only displayed
  // once per game.
  virtual wait<> display_woodcut( e_woodcut cut ) = 0;

  /* ============================================================
  ** Utilities
  ** ===========================================================*/

 public:
  // For convenience while developing, shouldn't really be used
  // to provide proper names for things for the player.
  static std::string identifier_to_display_name(
      std::string_view ident );

  // Returns the total number of windows that have been created
  // and displayed since the creation of the underlying window
  // manager on the backend of this interface. This can be used
  // to determine if a window was shown to the user in between
  // two points in time.
  virtual int total_windows_created() const = 0;
};

template<refl::ReflectedEnum E>
wait<maybe<E>> IGui::optional_enum_choice(
    EnumChoiceConfig const&               enum_config,
    refl::enum_map<E, std::string> const& names,
    refl::enum_map<E, bool> const&        disabled ) {
  ChoiceConfig config{ .msg  = enum_config.msg,
                       .sort = enum_config.sort };
  for( E item : refl::enum_values<E> )
    config.options.push_back( ChoiceConfigOption{
        .key = std::string( refl::enum_value_name( item ) ),
        .display_name = names[item],
        .disabled     = disabled[item] } );
  maybe<std::string> str_res =
      co_await optional_choice( config );
  if( !str_res.has_value() ) co_return nothing;
  UNWRAP_CHECK( res, refl::enum_from_string<E>( *str_res ) );
  co_return res;
}

template<refl::ReflectedEnum E>
wait<E> IGui::required_enum_choice(
    EnumChoiceConfig const&               enum_config,
    refl::enum_map<E, std::string> const& names,
    refl::enum_map<E, bool> const&        disabled ) {
  ChoiceConfig config{ .msg  = enum_config.msg,
                       .sort = enum_config.sort };
  for( E item : refl::enum_values<E> )
    config.options.push_back( ChoiceConfigOption{
        .key = std::string( refl::enum_value_name( item ) ),
        .display_name = names[item],
        .disabled     = disabled[item] } );
  std::string const str_res = co_await required_choice( config );
  UNWRAP_CHECK( res, refl::enum_from_string<E>( str_res ) );
  co_return res;
}

template<refl::ReflectedEnum E>
wait<maybe<E>> IGui::optional_enum_choice(
    EnumChoiceConfig const&               enum_config,
    refl::enum_map<E, std::string> const& names ) {
  co_return co_await optional_enum_choice(
      enum_config, names,
      /*disabled=*/refl::enum_map<E, bool>{} );
}

template<refl::ReflectedEnum E>
wait<E> IGui::required_enum_choice(
    EnumChoiceConfig const&               enum_config,
    refl::enum_map<E, std::string> const& names ) {
  co_return co_await required_enum_choice(
      enum_config, names,
      /*disabled=*/refl::enum_map<E, bool>{} );
}

template<refl::ReflectedEnum E>
wait<maybe<E>> IGui::optional_enum_choice(
    EnumChoiceConfig const&        enum_config,
    refl::enum_map<E, bool> const& disabled ) {
  refl::enum_map<E, std::string> names;
  for( E item : refl::enum_values<E> )
    names[item] = identifier_to_display_name(
        refl::enum_value_name( item ) );
  co_return co_await optional_enum_choice( enum_config, names,
                                           disabled );
}

template<refl::ReflectedEnum E>
wait<E> IGui::required_enum_choice(
    EnumChoiceConfig const&        enum_config,
    refl::enum_map<E, bool> const& disabled ) {
  refl::enum_map<E, std::string> names;
  for( E item : refl::enum_values<E> )
    names[item] = identifier_to_display_name(
        refl::enum_value_name( item ) );
  co_return co_await required_enum_choice( enum_config, names,
                                           disabled );
}

template<refl::ReflectedEnum E>
wait<maybe<E>> IGui::optional_enum_choice(
    EnumChoiceConfig const& enum_config ) {
  co_return co_await optional_enum_choice<E>(
      enum_config, /*disabled=*/refl::enum_map<E, bool>{} );
}

template<refl::ReflectedEnum E>
wait<E> IGui::required_enum_choice(
    EnumChoiceConfig const& enum_config ) {
  co_return co_await required_enum_choice<E>(
      enum_config, /*disabled=*/refl::enum_map<E, bool>{} );
}

template<refl::ReflectedEnum E>
wait<maybe<E>> IGui::optional_enum_choice(
    refl::enum_map<E, bool> const& disabled, bool sort ) {
  EnumChoiceConfig config{ .msg = "Select One", .sort = sort };
  co_return co_await optional_enum_choice<E>( config, disabled );
}

template<refl::ReflectedEnum E>
wait<E> IGui::required_enum_choice(
    refl::enum_map<E, bool> const& disabled, bool sort ) {
  EnumChoiceConfig config{ .msg = "Select One", .sort = sort };
  co_return co_await required_enum_choice<E>( config, disabled );
}

template<refl::ReflectedEnum E>
wait<maybe<E>> IGui::optional_enum_choice( bool sort ) {
  co_return co_await optional_enum_choice(
      refl::enum_map<E, bool>{}, sort );
}

template<refl::ReflectedEnum E>
wait<E> IGui::required_enum_choice( bool sort ) {
  co_return co_await required_enum_choice(
      refl::enum_map<E, bool>{}, sort );
}

template<refl::ReflectedEnum E>
wait<maybe<E>> IGui::partial_optional_enum_choice(
    std::string const& msg, std::vector<E> const& options,
    bool sort ) {
  ChoiceConfig config{ .msg = msg, .sort = sort };
  for( E item : options ) {
    auto key = std::string( refl::enum_value_name( item ) );
    config.options.push_back( ChoiceConfigOption{
        .key          = key,
        .display_name = identifier_to_display_name( key ) } );
  }
  maybe<std::string> str_res =
      co_await optional_choice( config );
  if( !str_res.has_value() ) co_return nothing;
  UNWRAP_CHECK( res, refl::enum_from_string<E>( *str_res ) );
  co_return res;
}

template<refl::ReflectedEnum E>
wait<maybe<E>> IGui::partial_optional_enum_choice(
    std::vector<E> const& options, bool sort ) {
  co_return co_await partial_optional_enum_choice(
      "Select One", options, sort );
}

} // namespace rn
