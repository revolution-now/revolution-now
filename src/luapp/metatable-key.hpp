/****************************************************************
**metatable-key.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-26.
*
* Description: Special key for representing metatables.
*
*****************************************************************/
#pragma once

namespace lua {

struct metatable_key_t {};

inline constexpr auto metatable_key = metatable_key_t{};

} // namespace lua
