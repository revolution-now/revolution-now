/****************************************************************
**font.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-22.
*
* Description: Enum for referring to fonts.
*
*****************************************************************/
#include "font.hpp"

// Revolution Now
#include "config-files.hpp"

// Revolution Now (config)
#include "../config/rcl/font.inl"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
namespace font {

e_font standard() { return config_font.game_default; }
e_font nat_icon() { return config_font.nat_icon; }
e_font small() { return config_font.small_font; }
e_font main_menu() { return config_font.main_menu; }

} // namespace font

} // namespace rn
