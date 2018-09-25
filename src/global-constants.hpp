/****************************************************************
* global-constants.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Global constants for the game.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include "typed-int.hpp"

#include <string_view>

namespace rn {

constexpr W g_tile_width{32};
constexpr H g_tile_height{32};

constexpr W g_world_max_width{1000};
constexpr H g_world_max_height{1000};

inline constexpr std::string_view g_window_title = "Revolution | Now";

} // namespace rn

