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

namespace rr {
struct ITextometer;
}

namespace rn {

struct Planes;
struct WindowPlane;

/****************************************************************
** RealGui
*****************************************************************/
// Presents real GUI elements to the player. For unit testing in-
// stead use a mock of IGui, not this one.
struct RealGui : IGui {
  RealGui( Planes& planes, rr::ITextometer const& textometer )
    : planes_( planes ), textometer_( textometer ) {}

  // Implement IGui.
  wait<> message_box( std::string const& msg ) override;

  // Implement IGui.
  void transient_message_box( std::string const& msg ) override;

  // Implement IGui.
  wait<std::chrono::microseconds> wait_for(
      std::chrono::microseconds time ) override;

  // Implement IGui.
  wait<> display_woodcut( e_woodcut cut ) override;

  // Implement IGui.
  int total_windows_created() const override;

 private:
  // The ones in this section should not be invoked directly, but
  // instead through the IGui wrappers that are public. That way
  // the required/optional status will be reflected in the return
  // type.

  // Implement IGui.
  wait<maybe<std::string>> choice(
      ChoiceConfig const& config ) override;

  // Implement IGui.
  wait<maybe<std::string>> string_input(
      StringInputConfig const& config ) override;

  // Implement IGui.
  wait<maybe<int>> int_input(
      IntInputConfig const& config ) override;

  // Implement IGui.
  wait<std::unordered_map<int, bool>> check_box_selector(
      std::string const& title,
      std::unordered_map<int, CheckBoxInfo> const& items )
      override;

  // Implement IGui.
  wait<ui::e_ok_cancel> ok_cancel_box( std::string const& title,
                                       ui::View& view ) override;

 private:
  WindowPlane& window_plane() const;

  Planes& planes_;
  rr::ITextometer const& textometer_;
};

} // namespace rn
