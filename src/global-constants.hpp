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

#include <string_view>

namespace rn {

constexpr int g_tile_width = 32;
constexpr int g_tile_height = 32;

constexpr int g_viewport_width_tiles = 48;
constexpr int g_viewport_height_tiles = 26;

inline constexpr std::string_view g_window_title = "Revolution|Now";

} // namespace rn

