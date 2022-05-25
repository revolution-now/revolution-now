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

// Rds
#include "igui.rds.hpp"

// base
#include "base/fmt.hpp"
#include "base/no-default.hpp"

// C++ standard library
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
};

} // namespace rn
