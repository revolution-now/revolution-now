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

// Rds
#include "plane-stack.rds.hpp"

// refl
#include "refl/query-enum.hpp"

// c++ standard library
#include <memory>
#include <string_view>
#include <vector>

namespace rn {

struct Planes;

/****************************************************************
** WindowPlane
*****************************************************************/
// The GUI methods in this class should be relatively primitive,
// since it is intended only to be used to implement the IGui in-
// terface, which is the more polished one.
struct WindowPlane {
  WindowPlane( Planes& planes, e_plane_stack where );
  ~WindowPlane() noexcept;

  wait<> message_box( std::string_view msg );

  template<typename First, typename... Rest>
  wait<> message_box( std::string_view msg, First&& first,
                      Rest&&... rest ) {
    return message_box( fmt::format(
        fmt::runtime( msg ), std::forward<First>( first ),
        std::forward<Rest>( rest )... ) );
  }

  wait<int> select_box(
      std::string_view                msg,
      std::vector<std::string> const& options );

  wait<maybe<std::string>> str_input_box(
      std::string_view title, std::string_view msg,
      std::string_view initial_text );

 private:
  friend struct Window;

  Planes&             planes_;
  e_plane_stack const where_;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace rn

namespace rn::ui {

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
    std::vector<UnitId> const& ids_, bool allow_activation );

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

struct IntInputBoxOptions {
  std::string_view title   = "";
  std::string_view msg     = "";
  maybe<int>       min     = nothing;
  maybe<int>       max     = nothing;
  maybe<int>       initial = nothing;
};

wait<maybe<int>> int_input_box(
    IntInputBoxOptions const& options );

} // namespace rn::ui
