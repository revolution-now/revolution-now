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

#include "typed-int.hpp"

#include <string_view>

namespace rn {

constexpr W g_tile_width{32};
constexpr H g_tile_height{32};

constexpr W g_screen_width_tiles{28}; //{48};
constexpr H g_screen_height_tiles{15}; //{26};

// At standard zoom, when tile size is (g_tile_width, g_tile_height);
constexpr W g_viewport_width_tiles{22}; //{40};
constexpr H g_viewport_height_tiles{14}; //{25};

constexpr W g_world_max_width{1000};
constexpr H g_world_max_height{1000};

inline constexpr std::string_view g_window_title = "Revolution | Now";

} // namespace rn

