/****************************************************************
**font.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-22.
*
* Description: Enum for referring to fonts.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "enum.hpp"

namespace rn {

enum class e_( font,
               _7_12_serif_16pt, //
               _6x6,             //
               habbo_15          //
);

namespace font {

e_font standard();
e_font nat_icon();

e_font small();
e_font main_menu();

} // namespace font

} // namespace rn
