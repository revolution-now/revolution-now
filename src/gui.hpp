/****************************************************************
**gui.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-24.
*
* Description: IGui implementation for creating a real GUI.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "igui.hpp"

namespace rn {

/****************************************************************
** RealGui
*****************************************************************/
// Presents real GUI elements to the player. For unit testing in-
// stead use a mock of IGui, not this one.
struct RealGui : IGui {
  // Displays a message box. Hitting basically any key or
  // clicking the mouse (anywhere) should close it.
  wait<> message_box( std::string_view msg ) override;

  // Display a message and one or more choices and let the user
  // choose one.
  wait<std::string> choice(
      ChoiceConfig const& config ) override;
};

} // namespace rn
