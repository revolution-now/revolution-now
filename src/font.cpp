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

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
namespace font {

// Until we add an OpenGL-compatible font system these are
// meaningless.
e_font standard() { return e_font::_6x6; }
e_font nat_icon() { return e_font::_6x6; }
e_font small() { return e_font::_6x6; }
e_font main_menu() { return e_font::_6x6; }

} // namespace font

} // namespace rn
