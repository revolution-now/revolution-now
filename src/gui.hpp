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

struct WindowPlane;

/****************************************************************
** RealGui
*****************************************************************/
// Presents real GUI elements to the player. For unit testing in-
// stead use a mock of IGui, not this one.
struct RealGui : IGui {
  RealGui( WindowPlane& window_plane )
    : window_plane_( window_plane ) {}

  // Implement IGui.
  wait<> message_box( std::string_view msg ) override;

  // Implement IGui.
  wait<std::chrono::microseconds> wait_for(
      std::chrono::microseconds time ) override;

  // Implement IGui.
  wait<> display_woodcut( e_woodcut cut ) override;

 private:
  // The ones in this section should not be invoked directly, but
  // instead through the IGui wrappers that are public. That way
  // the required/optional status will be reflected in the return
  // type.

  // Implement IGui.
  wait<maybe<std::string>> choice(
      ChoiceConfig const& config,
      e_input_required    required ) override;

  // Implement IGui.
  wait<maybe<std::string>> string_input(
      StringInputConfig const& config,
      e_input_required         required ) override;

  // Implement IGui.
  wait<maybe<int>> int_input(
      IntInputConfig const& config,
      e_input_required      required ) override;

 private:
  WindowPlane& window_plane_;
};

} // namespace rn
