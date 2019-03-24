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

// c++ standard library
#include <string_view>
#include <vector>

namespace rn {

struct Plane;
Plane* window_plane();

} // namespace rn

namespace rn::ui {

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
