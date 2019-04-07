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
#include "aliases.hpp"
#include "enum.hpp"
#include "id.hpp"

// c++ standard library
#include <string_view>
#include <vector>

namespace rn {

struct Plane;
Plane* window_plane();

} // namespace rn

namespace rn::ui {

void message_box( std::string_view msg );

enum class e_( unit_selection, //
               clear_orders,   //
               activate        // implies clear_orders
);

struct UnitSelection {
  UnitId           id;
  e_unit_selection what;
};

Vec<UnitSelection> unit_selection_box( Vec<UnitId> const& ids_,
                                       bool allow_activation );

/****************************************************************
** Simple Option-Select Window
*****************************************************************/
std::string select_box( std::string_view title,
                        Vec<Str>         options );

/****************************************************************
** Canned Option-Select Windows
*****************************************************************/
enum class e_confirm { yes, no };

e_confirm yes_no( std::string_view title );

/****************************************************************
** Testing Only
*****************************************************************/
void window_test();

} // namespace rn::ui
