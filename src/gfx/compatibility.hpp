/****************************************************************
**compatibility.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-04.
*
* Description: Some namespace injections for compatibility, to be
*              eventually removed.
*
*****************************************************************/
#pragma once

namespace gfx {

enum class e_cardinal_direction;
enum class e_cdirection;
enum class e_diagonal_direction;
enum class e_direction;
enum class e_direction_type;

} // namespace gfx

namespace rn {

using ::gfx::e_cardinal_direction;
using ::gfx::e_cdirection;
using ::gfx::e_diagonal_direction;
using ::gfx::e_direction;
using ::gfx::e_direction_type;

}
