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
#include "maybe.hpp"
#include "wait.hpp"

// base
#include "base/fmt.hpp"
#include "base/no-default.hpp"

// C++ standard library
#include <string>
#include <string_view>
#include <vector>

namespace rn {

// Used to configure a message box that presents the user with
// some choices.
struct ChoiceConfig {
  bool operator==( ChoiceConfig const& ) const = default;

  struct Option {
    std::string key          = base::no_default<>;
    std::string display_name = base::no_default<>;
  };

  // This text will be reflowed and may end up on multiple lines.
  std::string msg = base::no_default<>;

  // Needs to be a vector for well-defined and customizable or-
  // dering.
  std::vector<Option> options = base::no_default<>;

  // If this has a value then the user is allowed to hit escape
  // to close the window and in that case it will return the
  // given key, which does not have to be among those in the op-
  // tions.
  maybe<std::string> key_on_escape = nothing;
};

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

  // For convenience.  Should not be overridden.
  template<typename Arg, typename... Rest>
  wait<> message_box( std::string_view msg, Arg&& arg,
                      Rest&&... rest ) {
    return message_box( fmt::format(
        fmt::runtime( msg ), std::forward<Arg>( arg ),
        std::forward<Rest>( rest )... ) );
  }
};

} // namespace rn
