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
  // Implement IGui.
  wait<> message_box( std::string_view msg ) override;

  // Implement IGui.
  wait<std::string> choice(
      ChoiceConfig const& config ) override;

  // Implement IGui.
  wait<std::string> string_input(
      StringInputConfig const& config ) override;

  // Implement IGui.
  wait<std::chrono::microseconds> wait_for(
      std::chrono::microseconds time ) override;
};

} // namespace rn
