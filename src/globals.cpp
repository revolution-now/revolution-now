/****************************************************************
**globals.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Globals needed by the game.
*
*****************************************************************/
#include "globals.hpp"

#include "util.hpp"
#include "macros.hpp"

namespace rn {

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
W g_screen_width_tiles{11};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
H g_screen_height_tiles{6};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
W g_panel_width_tiles{6};

} // namespace

::SDL_Window*   g_window   = nullptr;
::SDL_Renderer* g_renderer = nullptr;
Texture         g_texture_world;

W screen_width_tiles() { //{28}; //{48};
  return g_screen_width_tiles;
}

H screen_height_tiles() { //{15}; //{26};
  return g_screen_height_tiles;
}

void set_screen_width_tiles( W w ) {
  CHECK( w > g_panel_width_tiles );
  g_screen_width_tiles = w;
}

void set_screen_height_tiles( H h ) {
  CHECK( h > 1 );
  g_screen_height_tiles = h;
}

W viewport_width_tiles() {
  return g_screen_width_tiles - g_panel_width_tiles;
}

H viewport_height_tiles() { return g_screen_height_tiles - 1; }

} // namespace rn
