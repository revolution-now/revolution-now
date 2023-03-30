/****************************************************************
**unit-flag.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-29.
*
* Description: Some logic related to rendering the flag
*              next to a unit.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

// rds
#include "unit-flag.rds.hpp"

namespace rn {

enum class e_nation;
enum class e_unit_type;

struct NativeUnit;
struct SSConst;
struct Unit;
struct unit_orders;

/****************************************************************
** Options.
*****************************************************************/
enum class e_flag_count {
  single,

  // This is used to visually indicate when there are multiple
  // units on a tile; as in the OG, we draw two stacked flags.
  multiple,
};

enum class e_flag_char_type {
  normal,

  // This is used when we are in the "show strategy" cheat mode.
  // In that mode, each AI-controlled unit gets a character on
  // their flag indicating what their current strategy is.
  strategy,
};

struct UnitFlagOptions {
  e_flag_count     flag_count = {};
  e_flag_char_type type       = {};

  // TODO: consider adding an rds feature to add these for each
  // field. The feature could be called "field_builders."
  [[nodiscard]] UnitFlagOptions&& with_flag_count(
      e_flag_count flag_count ) &&;
};

/****************************************************************
** Public API
*****************************************************************/
// Given a euro unit, return the info describing how to render
// its flag, assuming it will have one.
UnitFlagRenderInfo euro_unit_flag_render_info(
    Unit const& unit, maybe<e_nation> viewer,
    UnitFlagOptions const& options );

// Given a native unit, return the info describing how to render
// its flag, assuming it will have one.
UnitFlagRenderInfo native_unit_flag_render_info(
    SSConst const& ss, NativeUnit const& unit,
    UnitFlagOptions const& options );

// This is for when we are rendering a unit with a flag but it is
// not a real unit, so all that we have is the unit type and some
// other info.
UnitFlagRenderInfo euro_unit_type_orders_flag_info(
    e_unit_type unit_type, unit_orders const& orders,
    e_nation nation );

} // namespace rn
