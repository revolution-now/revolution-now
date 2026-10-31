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

namespace rn {

enum class e_font;

namespace font {

[[nodiscard]] e_font standard();
[[nodiscard]] e_font nat_icon();

[[nodiscard]] e_font small();
[[nodiscard]] e_font main_menu();

} // namespace font

} // namespace rn
