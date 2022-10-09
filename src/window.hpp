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
#include "enum.hpp"
#include "error.hpp"
#include "expect.hpp"
#include "ui-enums.hpp"
#include "unit-id.hpp"
#include "wait.hpp"

// gfx
#include "gfx/coord.hpp"

// refl
#include "refl/query-enum.hpp"

// c++ standard library
#include <memory>
#include <string_view>
#include <vector>

namespace rr {
struct Renderer;
}

namespace rn {

struct Plane;
struct UnitsState;
struct WindowManager;

namespace ui {
struct View;
}

/****************************************************************
** Window
*****************************************************************/
struct Window {
  Window( WindowManager& window_manager );
  // Removes this window from the window manager.
  ~Window() noexcept;

  void set_view( std::unique_ptr<ui::View> view );

  bool operator==( Window const& rhs ) const;

  void draw( rr::Renderer& renderer, Coord where ) const;

  // Tell the window manager to center this window.
  void center_me() const;

  // Run auto-padding recursively through all of the views.
  void autopad_me();

  Delta delta() const;

  // We need to pass in the upper left of the window because a
  // window does not know its position on screen.
  Rect rect( Coord origin ) const;

  Coord inside_border( Coord origin ) const;

  Rect inside_border_rect( Coord origin ) const;

  Coord inside_padding( Coord origin ) const;

  // ABS coord of upper-left corner of view.
  Coord view_pos( Coord origin ) const;

  std::unique_ptr<ui::View> const& view() const { return view_; }
  std::unique_ptr<ui::View>&       view() { return view_; }

 private:
  WindowManager&            window_manager_;
  std::unique_ptr<ui::View> view_;
};

/****************************************************************
** Helper Configs.
*****************************************************************/
struct IntInputBoxOptions {
  std::string_view msg      = "";
  maybe<int>       min      = nothing;
  maybe<int>       max      = nothing;
  maybe<int>       initial  = nothing;
  e_input_required required = e_input_required::no;
};

struct SelectBoxOption {
  std::string name    = {};
  bool        enabled = {};
};

/****************************************************************
** WindowPlane
*****************************************************************/
// The GUI methods in this class should be relatively primitive,
// since it is intended only to be used to implement the IGui in-
// terface, which is the more polished one.
struct WindowPlane {
  WindowPlane();
  ~WindowPlane();

  WindowManager& manager();

  wait<> message_box( std::string_view msg );

  template<typename First, typename... Rest>
  wait<> message_box( std::string_view msg, First&& first,
                      Rest&&... rest ) {
    return message_box( fmt::format(
        fmt::runtime( msg ), std::forward<First>( first ),
        std::forward<Rest>( rest )... ) );
  }

  // The result will be nothing iff required==no and the user
  // cancels (e.g. but hitting escape or clicking outside the
  // window).
  wait<maybe<int>> select_box(
      std::string_view                    msg,
      std::vector<SelectBoxOption> const& options,
      e_input_required required, maybe<int> initial_selection );

  wait<maybe<std::string>> str_input_box(
      std::string_view msg, std::string_view initial_text,
      e_input_required required );

  wait<maybe<int>> int_input_box(
      IntInputBoxOptions const& options );

 private:
  friend struct Window;

  struct Impl;
  std::unique_ptr<Impl> impl_;

 public:
  Plane& impl();
};

enum class e_unit_selection {
  clear_orders,
  activate // implies clear_orders
};

struct UnitSelection {
  UnitId           id;
  e_unit_selection what;
};
NOTHROW_MOVE( UnitSelection );

wait<std::vector<UnitSelection>> unit_selection_box(
    UnitsState const& units_state, WindowPlane& window_plane,
    std::vector<UnitId> const& ids_, bool allow_activation );

} // namespace rn

namespace rn::ui {

/****************************************************************
** Validators
*****************************************************************/
// These should probably be moved elsewhere.

using ValidatorFunc = std::function<bool( std::string const& )>;

// Makes a validator that enforces that the input be parsable
// into an integer and that (optionally) it is within [min, max].
ValidatorFunc make_int_validator( maybe<int> min,
                                  maybe<int> max );

/****************************************************************
** Windows
*****************************************************************/
wait<e_ok_cancel> ok_cancel( std::string_view msg );

template<base::Show... Args>
wait<e_ok_cancel> ok_cancel( std::string_view fmt_str,
                             Args&&... args ) {
  return ok_cancel( fmt::format(
      fmt::runtime( fmt_str ), std::forward<Args>( args )... ) );
}

} // namespace rn::ui
